#!/usr/bin/python
from __future__ import division
from collections import OrderedDict

import argparse
import os
import json
# Make it work for Python 2+3 and with Unicode
import io
try:
  to_unicode = unicode
except NameError:
  to_unicode = str

cpu_freq = 0

def create_json_object(source):
  json_object = {}
  json_object["ranks"] = []

  #get measurement files
  file_list = os.listdir(source)
  for item in file_list:
    json_rank = {}

    #determine mpi rank based on file name (rank_#)
    rank = item.split('_', 1)[1]
    rank = rank.rsplit('.', 1)[0]
    json_rank["id"] = rank
    json_rank["threads"] = []
    
    #open meaurement file
    file_name = str(source) + "/rank_" + str(rank) 
    try:
      rank_file = open(file_name, "r")
      lines = rank_file.readlines() #[2:]
    except IOError as ioe:
      print("Cannot open file {} ({})".format(file_name, repr(ioe)))
      return
    
    line_counter = 0

    #read lines from file
    for line in lines:
      if line_counter == 0:
        #determine cpu frequency
        global cpu_freq
        cpu_freq = int(line.split(':', 1)[1]) * 1000000
        #print cpu_freq
        line_counter = line_counter + 1
        continue
      if line_counter == 1:
        #skip second line
        line_counter = line_counter + 1
        continue

      thread_id = line.split(',', 1)[0]

      #remove thread_id from line
      line = line.split(',', 1)[1]
      
      #create array of regions and events
      #print line
      regions = []
      events = []
      tmp_regions = [i.strip() for i in line.split('>,')]
      for region in tmp_regions:
        name = region.split(':', 1)[0]
        #remove " and <
        name = name.replace("<", "")
        name = name.replace("\"", "")
        regions.append(name)
        events_string = region.split(':', 1)[1]
        #remove < and >
        events_string = events_string.replace("<", "")
        events_string = "{" +str(events_string.replace(">", "")) + "}"
        events.append(events_string)

      json_thread = {}
      json_thread["id"] = int(thread_id)
      json_thread["regions"] = []

      for region,event in zip(regions,events):
        #print event
        region_dict = {}
        region_dict["name"] = region
        region_dict["events"] = json.loads(event)
        json_thread["regions"].append(region_dict)

      json_rank["threads"].append(json_thread)
    rank_file.close()
    json_object["ranks"].append(json_rank)

  return json_object

class Sum_Counters(object):
  regions = OrderedDict()
  regions_last_rank_id = {}

  def add_region(self, rank_id, region, events=OrderedDict()):
    if region not in self.regions:
      #new region
      new_region_events = events.copy()
      new_region_events['Number of ranks'] = 1
      new_region_events['Number of threads'] = 1
      new_region_events['Number of processes'] = 1
      self.regions[region] = new_region_events.copy()
      self.regions_last_rank_id[region] = rank_id
    else:
      #add counter values to existing region
      known_events = self.regions[region].copy()
      new_events = events.copy()

      #increase number of ranks when rank_id has changed
      if self.regions_last_rank_id[region] == rank_id:
        new_events['Number of ranks'] = 0
      else:
        self.regions_last_rank_id[region] = rank_id
        new_events['Number of ranks'] = 1

      #always increase number of threads
      new_events['Number of threads'] = 1
      new_events['Number of processes'] = 1

      #add values
      for event_key,event_value in known_events.iteritems():
        if 'Number of' in event_key or 'count' in event_key:
          known_events[event_key] = event_value + new_events[event_key]
        else:
          known_events[event_key] = float(format(event_value + new_events[event_key], '.2f'))
      self.regions[region] = known_events.copy()

  def get_json(self):
    #calculate correct thread number (number of processes / number of ranks)
    for name in self.regions:
      events = self.regions[name]
      events['Number of threads'] = int(events['Number of processes'] / events['Number of ranks'])
    return self.regions


def sum_json_object(json):
  sum_cnt = Sum_Counters()
  for ranks in json['ranks']:
    for threads in ranks['threads']:
      for regions in threads['regions']:
        sum_cnt.add_region(ranks['id'], regions['name'], regions['events'])
  return sum_cnt.get_json()


def format_events(events):
  #keep order as declared
  format_events = OrderedDict()
  rt = 1.0

  #Region Count
  if 'REGION_COUNT' in events:
    format_events['Region count'] = int(events['REGION_COUNT'])
    del events['REGION_COUNT']
  #Real Time
  if 'CYCLES' in events:
    #TODO: read cpu frequency from json file
    rt = float(events['CYCLES']) / int(cpu_freq)
    format_events['Real time in s'] = float(format(rt, '.2f'))
    del events['CYCLES']
  #CPU Time
  if 'perf::TASK-CLOCK' in events:
    pt = float(events['perf::TASK-CLOCK']) / 1000000000
    format_events['CPU time in s'] = float(format(pt, '.2f'))
    del events['perf::TASK-CLOCK']
  #PAPI_TOT_INS and PAPI_TOT_CYC to calculate IPC
  if 'PAPI_TOT_INS' and 'PAPI_TOT_CYC' in events:
    ipc = float( int(events['PAPI_TOT_INS']) / int(events['PAPI_TOT_CYC']) )
    format_events['IPC'] = float(format(ipc, '.2f'))
    del events['PAPI_TOT_INS']
    del events['PAPI_TOT_CYC']
  #FLOPS
  if 'PAPI_FP_OPS' in events:
    mflops = (float(events['PAPI_FP_OPS']) / 1000000) / rt
    format_events['MFLOPS'] = float(format(mflops, '.2f'))
    del events['PAPI_FP_OPS']
  
  #read the rest
  for event_key,event_value in events.iteritems():
    value = float(event_value)
    format_events[event_key] = float(format(value, '.2f'))

  return format_events


def format_json_object(json):
  json_object = {}
  json_object['ranks'] = []

  for ranks in json['ranks']:
    #print ranks['id']
    json_rank = {}
    json_rank['id'] = ranks['id']
    json_rank['threads'] = []
    for threads in ranks['threads']:
      #print threads['id']
      json_thread = {}
      json_thread['id'] = threads['id']
      json_thread['regions'] = []
      for regions in threads['regions']:
        #print regions['name']
        region = {}
        region['name'] = regions['name']
        region['events'] = {}
        events = {}
        for event_key,event_value in regions['events'].iteritems():
          events[event_key] = event_value
        
        formated_events = format_events(events)

        region['events'] = formated_events
        json_thread['regions'].append(region)
      json_rank['threads'].append(json_thread)
    json_object["ranks"].append(json_rank)
    
  return json_object

def write_json_file(data, file_name):
  with io.open(file_name, 'w', encoding='utf8') as outfile:
    str_ = json.dumps(data,
                      indent=4, sort_keys=False,
                      separators=(',', ': '), ensure_ascii=False)
    outfile.write(to_unicode(str_))


def main(source, format):
  if (format == "json"):
    json = create_json_object(source)
    formated_json = format_json_object(json)
    write_json_file(formated_json, 'papi.json')

    #summarize data over threads and ranks
    sum_json = sum_json_object(formated_json)
    write_json_file(sum_json, 'papi_sum.json')
  else:
    print("Format not supported!")


def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument('--source', type=str, required=False, default="papi",
                      help='Measurement directory of raw data.')
  parser.add_argument('--format', type=str, required=False, default='json', 
                      help='Output format, e.g. json.')
  return parser.parse_args()


if __name__ == '__main__':
  args = parse_args()
  main(format=args.format,
       source=args.source)
