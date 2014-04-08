# Testing daemon functionality

import BoostBuild
from multiprocessing import Process
import time

t = BoostBuild.Tester(use_test_config=False)

def WaitAndKill():
    #TODO change "sleep" with smth
    time.sleep(2)
    t.run_build_system(["--daemon-stop"], status = 1)

def RunAndStop():
    t.run_build_system(["--daemon"])
    
t.write("a.cpp", """\
#include "a.h"
int main() {}
""")
t.write("a.h", "")
t.write("jamroot.jam", "exe a : a.cpp ;")



# Testing that 1st run of daemon performs build
p = Process(target=WaitAndKill)
p.start()
t.run_build_system(["--daemon"])
t.expect_addition("bin/$toolset/debug/a.exe")

# Testing that modifying source file will result in rebuilding
p2 = Process(target=RunAndStop)
p2.start()
time.sleep(2)
t.touch("a.cpp")
t.run_build_system(["--daemon"], status = 1)
t.expect_touch("bin/$toolset/debug/a.exe")
t.run_build_system(["--daemon-stop"], status = 1)
p2.join()

# Testing that modifying included file will result in rebuilding
p2 = Process(target=RunAndStop)
p2.start()
time.sleep(2)
t.touch("a.h")
t.run_build_system(["--daemon"], status = 1)
t.expect_touch("bin/$toolset/debug/a.exe")
t.run_build_system(["--daemon-stop"], status = 1)
p2.join()

# Testing that not modifying files will not result in rebuilding
p2 = Process(target=RunAndStop)
p2.start()
time.sleep(2)
t.run_build_system(["--daemon"], status = 1)
t.expect_nothing_more()
t.run_build_system(["--daemon-stop"], status = 1)
p2.join()



#t.run_build_system(["--daemon"])
#t.run_build_system([])
#t.expect_nothing_more()

#t.expect_touch(["a.cpp"])
#t.expect_content("123")


t.cleanup()
