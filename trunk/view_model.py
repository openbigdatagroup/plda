#!/usr/bin/python2.4
# Print a readable text of the model
# ./view_model.py model_file viewable_file

import sys, os
num_topics = 0
map = []
sum = []
word_sum = {}
for line in open(sys.argv[1]):
  sep = line.split("\t")
  word = sep[0]
  sep = sep[1].split()
  if num_topics == 0:
    num_topics = len(sep)
    for i in range(num_topics):
      map.append({})
      sum.append(0.0)
  for i in range(len(sep)):
    if float(sep[i]) > 1:
      map[i][word] = float(sep[i])
      if word_sum.has_key(word):
        word_sum[word] += float(sep[i])
      else:
        word_sum[word] = float(sep[i])
      sum[i] += float(sep[i])

for i in range(len(map)):
  for key in map[i].keys():
    map[i][key] = map[i][key]

for i in range(len(map)):
  x = sorted(map[i].items(), key=lambda(k, v):(v, k), reverse = True)
  print
  print "TOPIC: ", i, sum[i]
  print
  for key in x:
    print key[0], key[1]
