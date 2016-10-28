import os
import errno
import fcntl

def open_file_write_exclusive(file_name):
    """
    If file is available - returns file object.
    If file is locked - returns None.
    If error occured - throws error.
    Truncates opened file.
    """

    fl = open(file_name, 'w')

    try:        
        fcntl.flock(fl, fcntl.LOCK_EX | fcntl.LOCK_NB)

    except IOError as e:
        if e.errno == errno.EAGAIN:
            return None
        else:
            raise

    fl.truncate(0)

    return fl