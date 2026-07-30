#!/usr/bin/env python3
import os

count = os.cpu_count()
assert count and count > 0
print(f"cpu_test ok mode={'hardware' if os.environ.get('SHAER_HARDWARE') == '1' else 'contract'} cores={count}")
