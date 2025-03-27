#!/usr/bin/env python3

from enum import Enum
from functools import reduce
import argparse, copy, json, logging, sys

def parse_args():
    parser = argparse.ArgumentParser(
        description="""
        Deduces whether or not an audit log presents a valid total
        ordering of the partially ordered log reported by Oliver Twist.
        """
    )
    parser.add_argument(
        "-x", "--audit-log", type=argparse.FileType("r"),
        help="audit log to validate"
    )
    parser.add_argument(
        "-y", "--oliver-log", type=argparse.FileType("r"),
        help="Oliver's log to validate with"
    )
    parser.add_argument(
        "-l", "--logging", default="ERROR",
        choices=['debug', 'info', 'warning', 'error', 'critical', 'off'],
        help="set logging level for logs to stderr (default: info)"
    )
    args = parser.parse_args()

    if args.logging == "off":
        logging.disable(logging.CRITICAL)
    else:
        logging.basicConfig(
            stream=sys.stderr, level=args.logging.upper(),
            format='[%(levelname)s] %(message)s'
        )

    return args

class Event:
    def __init__(self, timestamp, rid):
        self.timestamp = float(timestamp)
        self.rid = int(rid)

    def __str__(self):
        name = type(self).__name__
        serialized = json.dumps(self, default=lambda x: vars(x), indent=4)
        return f"{name} {serialized}"

    def __repr__(self):
        return self.__str__()

    def replay(self):
        raise NotImplementedError()

class Load(Event):
    def __init__(self, timestamp, rid, infile, outfile):
        super().__init__(timestamp, rid)
        self.infile = infile
        self.outfile = outfile

class Unload(Event):
    def __init__(self, timestamp, rid, file):
        super().__init__(timestamp, rid)
        self.file = file

class Sleep(Event):
    def __init__(self, timestamp, rid, seconds):
        super().__init__(timestamp, rid)
        self.seconds = int(seconds)

class Connect(Event):
    def __init__(self, timestamp, rid):
        super().__init__(timestamp, rid)

class SendLine(Event):
    def __init__(self, timestamp, rid):
        super().__init__(timestamp, rid)

class SendHeaders(Event):
    def __init__(self, timestamp, rid):
        super().__init__(timestamp, rid)

class SendAll(Event):
    def __init__(self, timestamp, rid):
        super().__init__(timestamp, rid)

class SentBody(Event):
    def __init__(self, timestamp, rid):
        super().__init__(timestamp, rid)

class Received(Event):
    def __init__(self, timestamp, rid):
        super().__init__(timestamp, rid)

class Wait(Event):
    def __init__(self, timestamp, rid, status):
        super().__init__(timestamp, rid)
        self.status = int(status)

def parse_event_line(line):
    fields = [field.strip() for field in line.strip().split(",")]
    timestamp = fields[0]
    type = fields[1]
    event = None

    if type == "LOAD":
        event = Load(timestamp, 0, fields[2], fields[3])
    elif type == "UNLOAD":
        event = Unload(timestamp, 0, fields[2])
    elif type == "SLEEP":
        event = Sleep(timestamp, 0, fields[2])
    elif type == "CONNECT":
        event = Connect(timestamp, fields[2])
    elif type == "SEND_LINE":
        event = SendLine(timestamp, fields[2])
    elif type == "SEND_HEADERS":
        event = SendHeaders(timestamp, fields[2])
    elif type == "SENT_BODY":
        event = SentBody(timestamp, fields[2])
    elif type == "SEND_ALL":
        event = SendAll(timestamp, fields[2])
    elif type == "RECEIVED":
        event = Received(timestamp, fields[2])
    elif type == "WAIT":
        event = Wait(timestamp, fields[2], fields[3])

    return event

class Request:
    def __init__(self, rid, happened):
        self.rid = rid
        self.happened = happened

    def __repr__(self):
        name = type(self).__name__
        serialized = json.dumps(
            self,
            default=lambda x: list(x) if isinstance(x, set) else vars(x),
            indent=4
        )
        return f"{name} {serialized}"

def partially_order(events):
    requests = {}
    finished = set()

    for event in events:
        if isinstance(event, Load) or isinstance(event, Unload):
            continue
        if event.rid not in requests:
            assert isinstance(event, Connect)
            requests[event.rid] = Request(event.rid, finished.copy())
        elif isinstance(event, Wait):
            finished.add(event.rid)

    return requests

def validate_order(audit_log, requests):
    status = 0
    happened = set()

    for line in audit_log.readlines():
        _, _, _, rid = line.strip().split(",")
        expected = set(requests[int(rid)].happened)
        if not expected.issubset(happened):
            logging.error(f"{rid}: {expected} is not subset of {happened}")
            status = 1
        else:
            logging.info(f"{rid}: {expected} is subset of {happened}")
        happened.add(int(rid))

    return status

def main():
    args = parse_args()
    events = [parse_event_line(line) for line in args.oliver_log.readlines()]
    requests = partially_order(events)
    status = validate_order(args.audit_log, requests)
    sys.exit(status)

if __name__ == "__main__":
    main()
