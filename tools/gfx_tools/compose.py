#!/bin/python

# compose.py
# Split a gfx directory made of 1000s of little images and files into a set of tilesheets
# and a tile_config.json

import argparse
import copy
import json
import os
import string
import subprocess
import pyvips

FALLBACK = {
    "file": "fallback.png",
    "tiles": [],
    "ascii": [
    { "offset": 0, "bold": False, "color": "BLACK" },
    { "offset": 256, "bold": True, "color": "WHITE" },
    { "offset": 512, "bold": False, "color": "WHITE" },
    { "offset": 768, "bold": True, "color": "BLACK" },
    { "offset": 1024, "bold": False, "color": "RED" },
    { "offset": 1280, "bold": False, "color": "GREEN" },
    { "offset": 1536, "bold": False, "color": "BLUE" },
    { "offset": 1792, "bold": False, "color": "CYAN" },
    { "offset": 2048, "bold": False, "color": "MAGENTA" },
    { "offset": 2304, "bold": False, "color": "YELLOW" },
    { "offset": 2560, "bold": True, "color": "RED" },
    { "offset": 2816, "bold": True, "color": "GREEN" },
    { "offset": 3072, "bold": True, "color": "BLUE" },
    { "offset": 3328, "bold": True, "color": "CYAN" },
    { "offset": 3584, "bold": True, "color": "MAGENTA" },
    { "offset": 3840, "bold": True, "color": "YELLOW" }
    ]
}

# stupid stinking Python 2 versus Python 3 syntax
def write_to_json(pathname, data):
    with open(pathname, "w") as fp:
        try:
            json.dump(data, fp)
        except ValueError:
            fp.write(json.dumps(data))


def find_or_make_dir(pathname):
    try:
        os.stat(pathname)
    except OSError:
        os.mkdir(pathname)


class PngRefs(object):
    def __init__(self):
        # dict of pngnames to png numbers; used to control uniqueness
        self.pngname_to_pngnum = { "null_image": 0 }
        # dict of png absolute numbers to png names
        self.pngnum_to_pngname = { 0: "null_image" }
        self.pngnum = 0

    def convert_pngname_to_pngnum(self, index):
        new_index = []
        if isinstance(index, list):
            for pngname in index:
                if isinstance(pngname, dict):
                    sprite_ids = pngname.get("sprite")
                    valid = False
                    if isinstance(sprite_ids, list):
                        new_sprites = []
                        for sprite_id in sprite_ids:
                            if sprite_id != "no_entry":
                                new_sprites.append(self.pngname_to_pngnum[sprite_id])
                                valid = True
                        pngname["sprite"] = new_sprites
                    elif sprite_ids and sprite_ids != "no_entry":
                        pngname["sprite"] = self.pngname_to_pngnum[sprite_ids]
                        valid = True
                    if valid:
                        new_index.append(pngname)
                elif pngname != "no_entry":
                    new_index.append(self.pngname_to_pngnum[pngname])
        elif index and index != "no_entry":
            new_index.append(self.pngname_to_pngnum[index])
        if new_index and len(new_index) == 1:
            return new_index[0]
        return new_index

    def convert_tile_entry(self, tile_entry):
        fg_id = tile_entry.get("fg")
        if fg_id:
            tile_entry["fg"] = self.convert_pngname_to_pngnum(fg_id)

        bg_id = tile_entry.get("bg")
        if bg_id:
            tile_entry["bg"] = self.convert_pngname_to_pngnum(bg_id)

        add_tile_entrys = tile_entry.get("additional_tiles", [])
        for add_tile_entry in add_tile_entrys:
            self.convert_tile_entry(add_tile_entry)

        return tile_entry

class TilesheetData(object):
    def __init__(self, subdir, tileset_pathname, tileset_info, refs):
        self.ts_name = subdir.split("pngs_")[1] + ".png"
        self.ts_path = tileset_pathname + "/" + self.ts_name
        #print("parsing tilesheet {}".format(ts_name))
        self.subdir_path = tileset_pathname + "/" + subdir
        self.tile_entries = []
        self.row_num = 0
        self.width = -1
        self.height = -1
        self.offset_x = 0
        self.offset_y = 0
        for ts_info in tileset_info:
            ts_specs = ts_info.get(self.ts_name)
            if ts_specs:
                self.width = ts_specs.get("sprite_width", -1)
                self.height = ts_specs.get("sprite_height", -1)
                self.offset_x = ts_specs.get("sprite_offset_x", 0)
                self.offset_y = ts_specs.get("sprite_offset_y", 0)
                break
        self.row_pngs = ["null_image"]
        refs.pngnum += 1
        self.first_index = pngnum
        self.max_index = pngnum

    def standard(self, tileset_width, tileset_height):
        if self.offset_x or self.offset_y:
            return False
        return self.width == tileset_width and self.height == tileset_height

    def walk_dirs(self, refs):
        to_merge = []
        for subdir_fpath, dirnames, filenames in os.walk(self.subdir_path):
            #print("{} has dirs {} and files {}".format(subdir_fpath, dirnames, filenames))
            for filename in filenames:
                filepath = subdir_fpath + "/" + filename
                if filename.endswith(".png"):
                    pngname = filename.split(".png")[0]
                    if pngname in refs.pngname_to_pngnum or pngname == "no_entry":
                        print("skipping {}".format(pngname))
                        continue
                    to_merge.append(filepath)
                    if self.width < 0 or self.height < 0:
                        img = pyvips.Image.new_from_file(filepath, access='random')
                        self.width = img.width
                        self.height = img.height

                    refs.pngname_to_pngnum[pngname] = refs.pngnum
                    refs.pngnum_to_pngname[refs.pngnum] = pngname
                    refs.pngnum += 1
                elif filename.endswith(".json"):
                    with open(filepath, "r") as fp:
                        tile_entry = json.load(fp)
                        self.tile_entries.append(tile_entry)
        
        images = []
        for p in to_merge:
            img = pyvips.Image.new_from_file(p, access="random")
            images.append(img)

        tilesheet = pyvips.Image.arrayjoin(images, across=16)
        tilesheet.write_to_file(self.ts_path)

args = argparse.ArgumentParser(description="Merge all the individal tile_entries and pngs in a tileset's directory into a tile_config.json and 1 or more tilesheet pngs.")
args.add_argument("tileset_dir", action="store",
                  help="local name of the tileset directory under gfx/")
argsDict = vars(args.parse_args())

tileset_dirname = argsDict.get("tileset_dir", "")

tileset_pathname = tileset_dirname
if not tileset_dirname.startswith("gfx/"):
    tileset_pathname = "gfx/" + tileset_dirname

try:
    os.stat(tileset_pathname)
except KeyError:
    print("cannot find a directory {}".format(tileset_pathname))
    exit -1

refs = PngRefs()

tileset_info_path = tileset_pathname + "/tile_info.json"
tileset_width = 16
tileset_height = 16
tileset_info = [{}]
with open(tileset_info_path, "r") as fp:
    tileset_info = json.load(fp)
    tileset_width = tileset_info[0].get("width")
    tileset_height = tileset_info[0].get("height")

pngnum = 0
all_ts_data = []
all_subdirs = os.listdir(tileset_pathname)
all_subdirs.sort()

for subdir in all_subdirs:
    if subdir.find("pngs_") < 0:
        continue
    ts_data = TilesheetData(subdir, tileset_pathname, tileset_info, refs)
    ts_data.walk_dirs(refs)
    ts_data.max_index = refs.pngnum
    all_ts_data.append(ts_data)

#print("pngname to pngnum {}".format(json.dumps(refs.pngname_to_pngnum, indent=2)))
#print("pngnum to pngname {}".format(json.dumps(refs.pngnum_to_pngname, sort_keys=True, indent=2)))

tiles_new = []
    
for ts_data in all_ts_data:
    ts_tile_entries = []
    for tile_entry in ts_data.tile_entries:
        converted_tile_entry = refs.convert_tile_entry(tile_entry)
        ts_tile_entries.append(converted_tile_entry)
    ts_conf = {
        "file": ts_data.ts_name,
        "tiles": ts_tile_entries,
        "//": "range {} to {}".format(ts_data.first_index, ts_data.max_index)
    }
    if not ts_data.standard(tileset_width, tileset_height):
        ts_conf["sprite_width"] = ts_data.width
        ts_conf["sprite_height"] = ts_data.height
        ts_conf["sprite_offset_x"] = ts_data.offset_x
        ts_conf["sprite_offset_y"] = ts_data.offset_y

    #print("\tfinalizing tilesheet {}".format(ts_name))
    tiles_new.append(ts_conf)

tiles_new.append(FALLBACK)
conf_data = {
    "tile_info": [{
        "width": tileset_width,
        "height": tileset_height
    }],
    "tiles-new": tiles_new
}
tileset_confpath = tileset_pathname + "/" + "tile_config.json"
write_to_json(tileset_confpath, conf_data)
