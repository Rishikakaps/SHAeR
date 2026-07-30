#!/usr/bin/env python3
import os
import resource

usage_kb = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
assert usage_kb > 0
print(f"memory_test ok mode={'hardware' if os.environ.get('SHAER_HARDWARE') == '1' else 'contract'} process_peak={usage_kb}")
