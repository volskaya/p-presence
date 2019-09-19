#!/usr/bin/env python

# Copyright (C) 2018  ---

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import json

from pathlib import Path
from subprocess import call
from shlex import split

LIMIT = 134
JSON_PATH = Path("./languages.json")
REPOSITORY_USAGE = Path("./repository_usage.json")
COLOR_ICONS_DIR = Path("./color_icons")


def render_color(color):
    """Calls imagemagick's convert to render a 1024x1024 color"""
    path = str(COLOR_ICONS_DIR / color)

    print(f"Rendering {color}.jpg")
    call(split(f'convert -size 512x512 xc:"{color}" {path}.jpg'))


def main():
    languages = json.loads(JSON_PATH.read_text())
    usage = json.loads(REPOSITORY_USAGE.read_text())
    usage_sorted = sorted(usage, key=lambda x: usage[x], reverse=True)
    count = 0

    for lang in usage_sorted[:LIMIT]:
        count += 1
        render_color(languages[lang]["color"])

    print(f"{count} colors rendered")


main()
