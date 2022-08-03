#!/usr/bin/env python3

import sys
import io
import os
from time import sleep
import re
import subprocess

regex = r".+:.+(?P<uid>[a-f0-9]{8}) key(?P<key>[A-B]) block (?P<block>[0-9]+).+: (?P<nonces>.+)"

HISTORY = {}
FOUND_LIST = []


def read_tty(tty):
    UID = None

    for line in iter(tty.readline, None):
        response = None
        line = line.strip()
        if line != "":
            if ">:" in line:
                response = "log\n\r"
            elif "block" in line:
                try:
                    match = re.search(regex, line)
                    uid = match.group("uid")
                    key = match.group("key")
                    block = match.group("block")
                    nonces = match.group("nonces")
                except AttributeError as e:
                    print(f"Error: {e}")
                    print(line)
                    exit(1)

                if UID is None:
                    UID = uid

                if (key, block) not in FOUND_LIST:
                    if (key, block) not in HISTORY.keys():
                        HISTORY[(key, block)] = nonces
                    else:
                        run_mfkey32v2(
                            block,
                            key,
                            UID,
                            HISTORY[(key, block)].split(" ") + nonces.split(" "),
                        )
                        FOUND_LIST.append((key, block))
        else:
            sleep(0.1)

        if response is not None:
            tty.write(response)
            tty.flush()


def run_mfkey32v2(block, key_type, uid, nonces):
    cmd = ["./mfkey32v2", uid, *nonces]
    output = subprocess.check_output(cmd)
    for output_line in output.splitlines():
        output_line = output_line.decode("utf-8")
        if "Found Key: [" in output_line:
            key = output_line.split("[")[1].split("]")[0]
            print(f"Found key {key_type} for block {block}: {key}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: %s <tty>" % sys.argv[0])
        exit(1)

    try:
        tty = io.TextIOWrapper(
            io.FileIO(os.open(sys.argv[1], os.O_NOCTTY | os.O_RDWR), "r+")
        )
    except:
        print(f"Could not open {sys.argv[1]}")
        sys.exit(1)

    print("Put your Flipper on the reader twice to trigger...")
    print("hit CTRL-C to quit.")

    read_tty(tty)
