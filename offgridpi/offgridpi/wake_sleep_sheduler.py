from datetime import datetime, timedelta

class Regime:
    def getRemainingRunTimeSeconds(self, start_run):
        raise Exception("abstract")

    def getNextSleepTimeSeconds(self,start_run):
        raise Exception("abstract")


class CyclicRegime(Regime):
    def __init__(self,run_time = timedelta(seconds=600),sleep_time=timedelta(seconds=3600)):
        if run_time < 180:
            ## guard against sleep forever
            run_time = 180
        if not isinstance(run_time,timedelta):
            raise Exception("timedelta please")
        self.run_time = run_time
        self.sleep_time = sleep_time

    def getRemainingRunTimeSeconds(self, start_run):
        now = datetime.now()
        uptime = now - start_run
        if (uptime > self.run_time):
            return 0
        remaining = self.run_time - uptime
        return remaining.seconds()

    def getNextSleepTimeSeconds(self,start_run):
        return self.sleep_time.seconds()

class AlwaysRunRegime(Regime):
    def getRemainingRunTimeSeconds(self, start_run):
        return 10000
    def getNextSleepTimeSeconds(self,start_run):
        return 0

def parse_definition(ss):
    s = ss.lower()
    if s.startswith('c:'):
        splitted = s.split(':')
        runtime = int(splitted[1])
        sleeptime = int(splitted[2])
        r =CyclicRegime(timedelta(seconds=runtime),timedelta(seconds=sleeptime))
    elif s == '' or s == 'always':
        r = AlwaysRunRegime()
    else:
        r = AlwaysRunRegime()
    r.definition = ss
    return r
