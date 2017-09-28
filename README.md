# dud
## Disk Usage and Days Stale

This is a simple multi-threaded Windows commandline utility that recurses the directories supplied as arguments and returns a columnar text report showing their sizes and the number of days since any file/dir beneath them has been modified.

_Example:_

    c:\> dud.exe f:\4*
    
    Path                                          Files    Dirs  Size (MB)  Days Stale
    ---------------------------------------- ---------- ------- ---------- -----------
    f:\4258                                           1       3          0       4,644
    f:\4269                                      20,166   2,642   76,627.3           0
    f:\4890                                          93      54     168.35          37

## Purpose
The purpose of this utility is to aid us in identifying project directories that are good candidates to move over to archival (nearline) storage.

## Feature To Do List
- [ ] **Data-Link:** Parse the directory names to detect possible "project numbers" then do an SQL lookup on the Accounting/ERP system to get project status.  Use a commandline flag to trigger this behavior.
- [ ] **Wildcards:** Figure out how to handle multilevel wildcards.  apparent MS C++ will handle F:\5* and pre-parse for us, but wigs out when it gets  f:\5*\cad.* I hope I don't have to end up rolling my own recursive filesystem iterating wildcard expansion routine, Robert Sedgwick forbid!

### Caveat Emptor
I'm just beginning C++ so it is quite new to me and I haven't done any C programming in 19-years, having spent most of the last 15+ years in SQL and PHP, so be careful using any of my code, but PLEASE feel free to tear it up and let me know how to make it better.
