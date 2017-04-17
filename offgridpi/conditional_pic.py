import datetime
import os, os.path

target_dir = "/home/pi/photos"
checktime="mtime"


def get_age(filename):
    if checktime == 'mtime':
        return datetime.datetime.fromtimestamp(os.path.getmtime(filename))
    else:
        return datetime.datetime.fromtimestamp(os.path.getctime(filename))

def most_recent_time(dir):
    fs = [os.path.join(dir,x) for x in os.listdir(dir)]
    ages = [get_age(x) for x in fs]
    if len(ages) == 0:
        return datetime.datetime(year=1900,month=1,day=1)
    return max(ages)

def should_make_photo():
    mrt = most_recent_time(target_dir)
    now = datetime.datetime.now()
    delta = now - mrt
    if now.hour >= 23 or now.hour < 6:
        return False

    if (delta > datetime.timedelta(hours=1)):
        return True
    return False

def make_photo():
    now = datetime.datetime.now()
    fname = 'still-' + now.strftime('%y%m%d-%H%M%S') + '.jpg'
    os.system('raspistill -o /home/pi/photos/%s' % fname)
    os.system("sync")

if (should_make_photo()):
    make_photo()
