"""Workflow stall watchdog: prints one line per state change.

usage: python wf_watch.py <workflow transcript dir> [stall_minutes]
Emits:
  STALL <label> <agentId> idle=<min>   - transcript untouched for >= stall_minutes and no result in journal
  DONE  <label> <agentId>              - result appeared in journal
  START <label> <agentId>              - new agent started
Runs forever (poll every 60 s); meant to be run under Monitor and re-armed.
"""
import json
import os
import sys
import time

wf = sys.argv[1]
stall_min = float(sys.argv[2]) if len(sys.argv) > 2 else 20.0
seen_started = set()
seen_done = set()
stalled = set()


def read_journal():
    started, done = {}, set()
    p = os.path.join(wf, 'journal.jsonl')
    if not os.path.exists(p):
        return started, done
    with open(p, encoding='utf-8', errors='replace') as f:
        for line in f:
            try:
                d = json.loads(line)
            except Exception:
                continue
            t = d.get('type')
            aid = d.get('agentId')
            if t == 'started' and aid:
                started[aid] = d.get('label', '?')
            elif t == 'result' and aid:
                done.add(aid)
    return started, done


while True:
    try:
        started, done = read_journal()
        now = time.time()
        for aid, label in started.items():
            if aid not in seen_started:
                seen_started.add(aid)
                print(f'START {label} {aid}', flush=True)
            if aid in done and aid not in seen_done:
                seen_done.add(aid)
                stalled.discard(aid)
                print(f'DONE {label} {aid}', flush=True)
            if aid not in done:
                tp = os.path.join(wf, f'agent-{aid}.jsonl')
                if os.path.exists(tp):
                    idle = (now - os.path.getmtime(tp)) / 60.0
                    if idle >= stall_min and aid not in stalled:
                        stalled.add(aid)
                        print(f'STALL {label} {aid} idle={idle:.0f}min', flush=True)
                    elif idle < stall_min and aid in stalled:
                        stalled.discard(aid)
                        print(f'RESUMED {label} {aid}', flush=True)
    except Exception as e:  # keep watching
        print(f'WATCHDOG-ERROR {e}', flush=True)
    time.sleep(60)
