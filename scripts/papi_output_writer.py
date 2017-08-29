#!/usr/bin/python

import argparse
import os
import json
# Make it work for Python 2+3 and with Unicode
import io
try:
  to_unicode = unicode
except NameError:
  to_unicode = str


def create_json_object(source):
  json_object = {}
  json_object["ranks"] = []

  #get measurement files
  file_list = os.listdir(source)
  for item in file_list:
    json_rank = {}

    #determine mpi rank based on file name (rank_#.out)
    rank = item.split('_', 1)[1]
    rank = rank.rsplit('.', 1)[0]
    json_rank["id"] = int(rank)
    json_rank["threads"] = []
    
    #open meaurement file
    file_name = str(source) + "/rank_" + str(rank) + ".out" 
    try:
      rank_file = open(file_name, "r")
      #skip first line
      lines = rank_file.readlines()[1:]
    except IOError as ioe:
      print("Cannot open file {} ({})".format(file_name, repr(ioe)))
      return
    
    #read lines from file
    for line in lines:
      thread_id = line.split(',', 1)[0]

      #remove thread_id from line
      line = line.split(',', 1)[1]
      
      #create array of regions and events
      print line
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
        print event
        region_dict = {}
        region_dict["name"] = region
        region_dict["events"] = json.loads(event)
        json_thread["regions"].append(region_dict)

      json_rank["threads"].append(json_thread)
    rank_file.close()
    json_object["ranks"].append(json_rank)

  return json_object

def write_json_file(data):
  with io.open('papi.json', 'w', encoding='utf8') as outfile:
    str_ = json.dumps(data,
                      indent=4, sort_keys=False,
                      separators=(',', ': '), ensure_ascii=False)
    outfile.write(to_unicode(str_))


def main(source, format):
  if (format == "json"):
    json = create_json_object(source)
    write_json_file(json)
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