import sys
import os.path

headers = []
records = []

for line in sys.stdin:
    fields = line.replace('\n', '').split(' ')
    distilled = map(lambda f: f.split('=')[1], fields[2:])
    headers = headers + [ ','.join(map(lambda f: f.split('=')[0], fields[2:])) ]
    distilled[0] = os.path.basename(distilled[0])[:-4]
    records = records + [ ','.join(distilled) ]

headers_as_list = list(set(headers))
assert len(headers_as_list) == 1
#header = headers_as_list[0].replace('random-actions', 'ra').replace('simulator', 'sim').replace('features', 'feat').replace('novelty-subtables', 'sub').replace('avg-time-per-decision', 'avg-time-dec').replace('avg-time-per-frame', 'avg-time-frame').replace('-', '.').replace('time.budget', 'budget')
header = headers_as_list[0]
print header
for r in records:
    print r

