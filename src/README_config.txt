PB_WRITE_DIRTYCL - write a text file with the dirty cache lines. 
NO_DIRTY_TRACKING - ptrace pause and resume, but do nothing in the tracker.
NO_DIRTY_TRACKING_SLEEP - ptrace pause and resume; instead of doing nothing, sleep for a duration. 
WPROTECT - write protect all pages before resuming the app's execution
PB_WRITE_TEXT_OUTPUT
PB_WRITE_ITER_STATS - print out iterations and dirty cache lines   
PB_WRITE_DCL - write DCL (dirty cache lines) file

cl.config - diff pages and report both number and addresses of dirty cache lines 
wp.config - writeprotect pages instead of diffing the pages
report_times.config - report application running times and number of dirty cache lines, but do not report dirty cache lines addresses
notracking.config - pause/resume app using KTracker, but do nothing while app is paused
sleep.config - pause/resume app using KTracker, and sleep while app is paused
