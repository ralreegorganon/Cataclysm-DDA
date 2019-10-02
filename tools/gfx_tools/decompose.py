#!/bin/python

# decompose.py
# Split a gfx tile_config.json into 1000s of little directories, each with their own config
# file and tile image.

import argparse
import copy
import json
import os
import subprocess
import pyvips

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


class TileSheetData(object):
    def __init__(self, tilesheet_data, default_width, default_height):
        self.ts_filename = tilesheet_data.get("file", "")
        self.tile_id_to_tile_entrys = {}
        self.pngnum_min = 10000000
        self.pngnum_max = 0
        self.sprite_height = tilesheet_data.get("sprite_height", default_height)
        self.sprite_width = tilesheet_data.get("sprite_width", default_width)
        self.sprite_offset_x = tilesheet_data.get("sprite_offset_x", 0)
        self.sprite_offset_y = tilesheet_data.get("sprite_offset_y", 0)
        self.expansions = []
        self.write_dim = self.sprite_height != default_height or self.sprite_width != default_width

    def check_for_expansion(self, tile_entry):
        if tile_entry.get("fg", -10) == 0:
            self.expansions.append(tile_entry)
            return True
        return False

    def parse_id(self, tile_entry):
        all_tile_ids = []
        read_tile_ids = tile_entry.get("id")
        if isinstance(read_tile_ids, list):
            for tile_id in read_tile_ids:
                if tile_id and tile_id not in all_tile_ids:
                    all_tile_ids.append(tile_id)
        elif read_tile_ids and read_tile_ids not in all_tile_ids:
            all_tile_ids.append(read_tile_ids)
        # print("tile {}".format(all_tile_ids[0]))
        if not all_tile_ids:
            return "background", ["background"]
        return all_tile_ids[0], all_tile_ids

    def parse_index(self, read_pngnums, all_pngnums, background_pngnums):
        local_pngnums = []
        if isinstance(read_pngnums, list):
            for pngnum in read_pngnums:
                if isinstance(pngnum, dict):
                    sprite_ids = pngnum.get("sprite", -10)
                    if isinstance(sprite_ids, list):
                        for sprite_id in sprite_ids:
                            if sprite_id >= 0 and sprite_id not in all_pngnums:
                                all_pngnums.append(sprite_id)
                            if sprite_id >= 0 and sprite_id not in local_pngnums:
                                local_pngnums.append(sprite_id)
                    else:
                        if sprite_ids >= 0 and sprite_ids not in all_pngnums:
                            all_pngnums.append(sprite_ids)
                        if sprite_ids >= 0 and sprite_ids not in local_pngnums:
                            local_pngnums.append(sprite_ids)
                else:
                    if pngnum >= 0 and pngnum not in all_pngnums:
                        all_pngnums.append(pngnum)
                    if pngnum >= 0 and pngnum not in local_pngnums:
                        local_pngnums.append(pngnum)
        else:
            if read_pngnums >= 0 and read_pngnums not in all_pngnums:
                all_pngnums.append(read_pngnums)
            if read_pngnums >= 0 and read_pngnums not in local_pngnums:
                local_pngnums.append(read_pngnums)
        if background_pngnums is not None:
            for local_pngnum in local_pngnums:
                if local_pngnum not in background_pngnums:
                    background_pngnums.append(local_pngnum)
        return all_pngnums

    def parse_png(self, tile_entry, refs):
        all_pngnums = []
        fg_id = tile_entry.get("fg", -10)
        all_pngnums = self.parse_index(fg_id, all_pngnums, None)

        bg_id = tile_entry.get("bg", -10)
        all_pngnums = self.parse_index(bg_id, all_pngnums, refs.background_pngnums)

        add_tile_entrys = tile_entry.get("additional_tiles", [])
        for add_tile_entry in add_tile_entrys:
            add_pngnums = self.parse_png(add_tile_entry, refs)
            for add_pngnum in add_pngnums:
                if add_pngnum not in all_pngnums:
                    all_pngnums.append(add_pngnum)
        # print("\tpngs: {}".format(all_pngnums))
        return all_pngnums

    def parse_tile_entry(self, tile_entry, refs):
        if self.check_for_expansion(tile_entry):
            return None
        tile_id, all_tile_ids = self.parse_id(tile_entry)
        if not tile_id:
            # print("no id for {}".format(tile_entry))
            return None
        tile_id = tile_id.replace("/", "_")
        all_pngnums = self.parse_png(tile_entry, refs)
        offset = 0
        for i in range(0, len(all_pngnums)):
            pngnum = all_pngnums[i]
            if pngnum in refs.pngnum_to_pngname:
                continue
            pngname = "{}_{}".format(tile_id, i + offset)
            while pngname in refs.pngname_to_pngnum:
                offset += 1
                pngname = "{}_{}".format(tile_id, i + offset)
            try:
                refs.pngnum_to_pngname.setdefault(pngnum, pngname)
                refs.pngname_to_pngnum.setdefault(pngname, pngnum)
            except TypeError:
                print("failed to parse {}".format(json.dumps(tile_entry, indent=2)))
                raise
            if pngnum > 0 and pngnum not in refs.background_pngnums:
                self.pngnum_max = max(self.pngnum_max, pngnum)
                self.pngnum_min = min(self.pngnum_min, pngnum)
        return tile_id

    def summarize(self, tilesheet_to_data, tile_info):
        #debug statement to verify pngnum_min and pngnum_max
        #print("{} from {} to {}".format(ts_filename, pngnum_min, pngnum_max))
        if self.pngnum_max > 0:
            self.pngnum_min = 16 * (self.pngnum_min // 16)
            tilesheet_to_data[self.ts_filename] = {
                "min": self.pngnum_min,
                "max": self.pngnum_max,
                "height": self.sprite_height,
                "width": self.sprite_width,
                "off_x": self.sprite_offset_x,
                "off_y": self.sprite_offset_y,
                "expansions": self.expansions
            }
            ts_tile_info = {}
            if self.sprite_offset_x or self.sprite_offset_y:
                ts_tile_info["sprite_offset_x"] = self.sprite_offset_x
                ts_tile_info["sprite_offset_y"] = self.sprite_offset_y
            if self.write_dim:
                ts_tile_info["sprite_width"] = self.sprite_width
                ts_tile_info["sprite_height"] = self.sprite_height
            if ts_tile_info:
                tile_info.append({self.ts_filename: ts_tile_info})
            #debug statement to verify final tilesheet data
        #print("{}: {}".format(self.ts_filename, json.dumps(tilesheet_to_data[self.ts_filename], indent=2)))


class ExtractionData(object):
    def __init__(self, tilesheet_to_data, tileset_pathname, ts_filename):
        ts_data = tilesheet_to_data.get(ts_filename,{})
        self.png_height = ts_data.get("height")
        self.png_width = ts_data.get("width")
        self.valid = False

        if not self.png_height or not self.png_width:
            return

        self.valid = True
        self.geometry_dim = "{}x{}".format(self.png_width, self.png_height)
        self.first_file_index = ts_data.get("min", 0)
        self.final_file_index = ts_data.get("max", 0)
        self.index_range = self.final_file_index - self.first_file_index
        self.offset_x = ts_data.get("offset_x", 0)
        self.offset_y = ts_data.get("offset_y", 0)
        self.expansions = ts_data.get("expansions", [])

        ts_base = ts_filename.split(".png")[0]
        self.ts_pathname = tileset_pathname + "/" + ts_filename
        self.ts_dir_pathname = tileset_pathname + "/pngs_" +  ts_base + "_{}".format(self.geometry_dim)
        find_or_make_dir(self.ts_dir_pathname)
        self.tilenum_in_dir = 256
        self.dir_count = 0
        self.subdir_pathname = ""

    def write_expansions(self):
        for expand_entry in self.expansions:
            expansion_id = expand_entry.get("id", "expansion")
            if not isinstance(expansion_id, str):
                continue
            expand_entry_pathname = self.ts_dir_pathname + "/" + expansion_id + ".json"
            write_to_json(expand_entry_pathname, expand_entry)

    def increment_dir(self):
        if self.tilenum_in_dir > 255:
            self.subdir_pathname = self.ts_dir_pathname + "/" + "images{}".format(self.dir_count)
            find_or_make_dir(self.subdir_pathname)
            self.tilenum_in_dir = 0
            self.dir_count += 1
        else:
            self.tilenum_in_dir += 1
        return self.subdir_pathname

    def extract_image(self, png_index, refs):
        if not png_index or refs.extracted_pngnums.get(png_index):
            return
        if png_index not in refs.pngnum_to_pngname:
            return
        pngname = refs.pngnum_to_pngname[png_index]
        self.increment_dir()
        file_index = png_index - self.first_file_index
        y_index = file_index // 16
        x_index = file_index - y_index * 16
        file_off_x = self.png_width * x_index + self.offset_x
        file_off_y = self.png_height * y_index + self.offset_y
        tile_png_pathname = self.subdir_pathname + "/" + pngname + ".png"

        img = pyvips.Image.new_from_file(self.ts_pathname, access='random')
        out = img.extract_area(file_off_x, file_off_y, self.png_width, self.png_height)
        out.pngsave(tile_png_pathname)
        refs.extracted_pngnums[png_index] = True

    def write_images(self, refs):
        for pngnum in range(self.first_file_index, self.final_file_index + 1):
            out_data.extract_image(pngnum, refs)


class PngRefs(object):
    def __init__(self):
        # dict of png absolute numbers to png names
        self.pngnum_to_pngname = {}
        # dict of pngnames to png numbers; used to control uniqueness
        self.pngname_to_pngnum = {}
        # list of background png_nums
        self.background_pngnums = []
        # list of pngs written out
        self.extracted_pngnums = {}

    def convert_index(self, read_pngnums, new_index, new_id):
        if isinstance(read_pngnums, list):
            for pngnum in read_pngnums:
                if isinstance(pngnum, dict):
                    sprite_ids = pngnum.get("sprite", -10)
                    if isinstance(sprite_ids, list):
                        new_sprites = []
                        for sprite_id in sprite_ids:
                            if sprite_id >= 0:
                                if not new_id:
                                    new_id = self.pngnum_to_pngname[sprite_id]
                                new_sprites.append(self.pngnum_to_pngname[sprite_id])
                        pngnum["sprite"] = new_sprites
                    else:
                        if sprite_ids >= 0:
                            if not new_id:
                                new_id = self.pngnum_to_pngname[sprite_ids]
                            pngnum["sprite"] = self.pngnum_to_pngname[sprite_ids]
                    new_index.append(pngnum)
                else:
                    if not new_id:
                        new_id = self.pngnum_to_pngname[pngnum]
                    new_index.append(self.pngnum_to_pngname[pngnum])
        elif read_pngnums >= 0:
            if not new_id:
                new_id = self.pngnum_to_pngname[read_pngnums]
            new_index.append(self.pngnum_to_pngname[read_pngnums])
        return new_id

    def convert_pngnum_to_pngname(self, tile_entry):
        new_fg = []
        new_id = ""
        fg_id = tile_entry.get("fg", -10)
        new_id = self.convert_index(fg_id, new_fg, new_id)
        bg_id = tile_entry.get("bg", -10)
        new_bg = []
        new_id = self.convert_index(bg_id, new_bg, new_id)
        add_tile_entrys = tile_entry.get("additional_tiles", [])
        for add_tile_entry in add_tile_entrys:
            self.convert_pngnum_to_pngname(add_tile_entry)
        tile_entry["fg"] = new_fg
        tile_entry["bg"] = new_bg

        return new_id, tile_entry

    def report_missing(self):
        for pngnum in self.pngnum_to_pngname:
            if not self.extracted_pngnums.get(pngnum):
                print("missing index {}, {}".format(pngnum, self.pngnum_to_pngname[pngnum]))


args = argparse.ArgumentParser(description="Split a tileset's tile_config.json into a directory per tile containing the tile data and png.")
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

tileset_confname = tileset_pathname + "/" + "tile_config.json"

try:
    os.stat(tileset_confname)
except KeyError:
    print("cannot find a directory {}".format(tileset_confname))
    exit -1

with open(tileset_confname) as conf_file:
    all_tiles = json.load(conf_file)

# dict of tilesheet png filenames to tilesheet data
tilesheet_to_data = {}
# dict of tilesheet filenames to arbitrary tile_ids to tile_entries
file_tile_id_to_tile_entrys = {}

refs = PngRefs()

all_tilesheet_data = all_tiles.get("tiles-new", [])
default_height = 0
default_width = 0
tile_info = all_tiles.get("tile_info", {})
if tile_info:
    default_width = tile_info[0].get("width")
    default_height = tile_info[0].get("height")

overlay_ordering = all_tiles.get("overlay_ordering", [])

for tilesheet_data in all_tilesheet_data:
    ts_data = TileSheetData(tilesheet_data, default_width, default_height)

    tile_id_to_tile_entrys = {}
    all_tile_entry = tilesheet_data.get("tiles", [])
    for tile_entry in all_tile_entry:
        tile_id = ts_data.parse_tile_entry(tile_entry, refs)
        if tile_id:
            tile_id_to_tile_entrys.setdefault(tile_id, [])
            tile_id_to_tile_entrys[tile_id].append(tile_entry)
    ts_data.summarize(tilesheet_to_data, tile_info)
    file_tile_id_to_tile_entrys[ts_data.ts_filename] = tile_id_to_tile_entrys

#debug statements to verify pngnum_to_pngname and pngname_to_pngnum
#print("pngnum_to_pngname: {}".format(json.dumps(refs.pngnum_to_pngname, sort_keys=True, indent=2)))
#print("pngname_to_pngnum: {}".format(json.dumps(refs.pngname_to_pngnum, sort_keys=True, indent=2)))
#print("{}".format(json.dumps(file_tile_id_to_tile_entrys, indent=2)))

for ts_filename, tile_id_to_tile_entrys in file_tile_id_to_tile_entrys.items():
    out_data = ExtractionData(tilesheet_to_data, tileset_pathname, ts_filename)

    if not out_data.valid:
        continue
    out_data.write_expansions()

    for tile_id, tile_entrys in tile_id_to_tile_entrys.items():
        #print("tile id {} with {} entries".format(tile_id, len(tile_entrys)))
        subdir_pathname = out_data.increment_dir()
        for tile_entry in tile_entrys:
            tile_entry_name, tile_entry = refs.convert_pngnum_to_pngname(tile_entry)
            tile_entry_pathname = subdir_pathname + "/" + tile_entry_name + ".json"
            write_to_json(tile_entry_pathname, tile_entry)
    out_data.write_images(refs)

if tile_info:
    tile_info_pathname = tileset_pathname + "/" + "tile_info.json"
    write_to_json(tile_info_pathname, tile_info)

refs.report_missing()
