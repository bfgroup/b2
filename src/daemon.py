import dbus
import dbus.service
import gobject
import bjam
from dbus.mainloop.glib import DBusGMainLoop
import time
import os   
import sys

# Checks if daemon is running. Stops daemon if --daemon-stop found in argv.
def check_for_daemon(daemon_dbus_name, daemon_dbus_iface):
    def output_parser():
            import time
            pipein = os.open(daemon_dbus_name, os.O_RDONLY)
            a = os.fdopen(pipein)
            #os.dup2(pipein,0)
            line = a.readline()
            while (not (line == daemon_dbus_name)):
                print  line.replace("\n","")
                line = a.readline()
            os.unlink(daemon_dbus_name)

    from multiprocessing import Process
    p = Process(target=output_parser)
    
    try:
        try_bus = dbus.SessionBus()
        daemon_object = try_bus.get_object(daemon_dbus_name, "/CommandHandler")
        iface = dbus.Interface(daemon_object, daemon_dbus_iface)

        if "--daemon-stop" in sys.argv:
            iface.Exit()
            print "daemon stopped"
            return 1

        os.mkfifo(daemon_dbus_name)

        

        p.start()
        res = iface.check_args(sys.argv)
        p.join()
        if res:
            #print "Work passed to daemon"
            return 1
    except dbus.DBusException as e:
        if (e.get_dbus_name() == "org.freedesktop.DBus.Error.NoReply"):
            p.join()
            return 1
        else:
            print "No daemon found"
    finally:
        try_bus.close()
    if "--daemon-stop" in sys.argv:
        return 1
    return 0


# Function implements process, that runs inotify and sends signals from it to main daemon by dbus
def inotify_handler (dir, daemon_dbus_name, daemon_dbus_iface):
    print "Inotify process started"
    print "Inotify dir: ", dir
    try:
        import pyinotify
    except Exception:
        print "install pyinotify firstly"
        print "you can get here: https://github.com/seb-m/pyinotify"
        return

    # Number of retries of connections to dbus
    num_retry = 3
    time.sleep(0.5)

    # Inotify should be interested only in file modifications
    wm = pyinotify.WatchManager()
    inotify_mask = pyinotify.IN_MODIFY | pyinotify.IN_ATTRIB | pyinotify.IN_MOVED_TO
    
    # Handling of inotify signals. Call file_changed function in main daemon via dbus interface
    class InotifyProcess(pyinotify.ProcessEvent):
        def process_IN_MODIFY(self, event):
            iface.file_changed(os.path.join(event.path, event.name))
        def process_IN_ATTRIB(self, event):
            iface.file_changed(os.path.join(event.path, event.name))
        def process_IN_MOVED_TO(self, event):
            iface.file_changed(os.path.join(event.path, event.name))
                    
    # Looking for changes only in passed dir. Recursively
    notifier = pyinotify.Notifier(wm, InotifyProcess())
    wm.add_watch(dir, inotify_mask, rec=True)
    
    # Trying to connect to dbus in a loop. If connection is successfull, start inotife
    while num_retry:
        try:
            daemon_bus = dbus.SessionBus()
            daemon_object = daemon_bus.get_object(daemon_dbus_name, "/InotifyHandler")
            iface = dbus.Interface(daemon_object, daemon_dbus_iface)
            print "Inotify process connected to dbus" 
            notifier.loop()
        except dbus.DBusException as e:
            num_retry += 1
            #print "Inotify process error: " + str(e)   
        num_retry -= 1
        time.sleep(1)

    print "Inotify process shut down"
    return

# Runs inotify daemon
def inotify_starter(dir, daemon_dbus_name, daemon_dbus_iface):
    from multiprocessing import Process
    p = Process(target=inotify_handler, args=(dir, daemon_dbus_name, daemon_dbus_iface,))
    p.daemon = True
    p.start()

# Runs main daemon
def daemon_starter(daemon_dbus_name):
    daemon_dbus_iface = "org.boost.boost_build.daemon_iface"
    refrlist = [0]

    # Class that is exported to dbus by main. Command interactions with daemon happens via this class    
    class CommandHandler(dbus.service.Object):
                def __init__(self, object_path, args, refrlist):
                    self.refrlist=refrlist
                    self.args = args                    
                    dbus.service.Object.__init__(self,  dbus.SessionBus(), object_path)
                    
                # Compare sys.argv with given args. Rebuild project if args are equal. Else, stop daemon.   
                @dbus.service.method(daemon_dbus_iface,
                                 in_signature='as', out_signature='b')
                def check_args(self, args):
                    pipeout = os.open(daemon_dbus_name, os.O_WRONLY, 0)
                    tmpdesc = os.dup( sys.stdout.fileno())
                    os.dup2( pipeout, sys.stdout.fileno())

                    if "--daemon-output" in sys.argv:
                        print "Files changed since last run:", self.refrlist[1:]

                    # Check if args has been changed 
                    args2 = []
                    for i in args:
                        args2.append(str(i))

                    for i, data in enumerate(args2):
                        if data == "--daemon-output":
                            del args2[i]

                    for i, data in enumerate(self.args):
                        if data == "--daemon-output" or data == "--daemon-second":
                            del self.args[i]

                    if args2 == self.args:
                        args2 = []
                    else:
                        self.args = args2

                    # Check if Jamfile's has been changed
                    jams = []
                    for s in self.refrlist[1:]:
                        tmp = os.path.basename(s).lower()
                        if (tmp == "jamfile.jam" or tmp == "jamroot.jam"):
                            jams.append(os.path.dirname(os.path.abspath(s)))
                        
                    if jams or args2:
                        from b2.build_system import main_daemon
                        main_daemon(jams,args2)
                    elif self.refrlist[1:]:
                        bjam.call("REFRESH",self.refrlist[1:])
                        bjam.call("UPDATE_NOW", "all")
    
                    del self.refrlist[ : ]
                    self.refrlist.append(time.time())
                    
                    os.write(pipeout, daemon_dbus_name)
                    os.close(pipeout)
                    os.dup2(tmpdesc, sys.stdout.fileno())
                    return 1
                    
                @dbus.service.method(daemon_dbus_iface,
                                     in_signature='', out_signature='')
                def Exit(self):
                    mainloop.quit()

    # Class that is exported to dbus by main. Inotify interactions with daemon happens via this class   
    class InotifyHandler(dbus.service.Object):
                def __init__(self, object_path, args, refrlist):
                    self.args = args
                    self.refrlist = refrlist
                    dbus.service.Object.__init__(self,  dbus.SessionBus(), object_path)
                        
                @dbus.service.method(daemon_dbus_iface,
                                     in_signature='s', out_signature='')
                def file_changed(self, name):
                    if (time.time() - self.refrlist[0] < 1):
                        return
                    name = str(os.path.relpath(name))
                    if not name in self.refrlist:
                        self.refrlist.append(name)
                    #print "InotifyHandler: Some file added. New file list:"
                    #print self.refrlist[1:]
                    return
    
    session_bus = dbus.SessionBus()
    name = dbus.service.BusName(daemon_dbus_name, session_bus)
    object = CommandHandler('/CommandHandler', sys.argv, refrlist)
    object = InotifyHandler('/InotifyHandler', sys.argv, refrlist)
    mainloop = gobject.MainLoop()
    print "Main daemon started"
    mainloop.run()
