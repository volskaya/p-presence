#!/usr/bin/env python

# Copyright (C) github.com/volskaya

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

import asyncio
import aiohttp
import time
import json
import re

from pathlib import Path

TOKEN = Path("./token")
JSON_PATH = Path("./languages.json")
COUNTED_JSON = Path("./repository_usage.json")
SUMMARY_JSON = Path("./repository_usage_summary.json")
GITHUB = "https://api.github.com"
LANGUAGE_SEARCH = GITHUB + "/search/repositories?q=language:{}"
SEARCH = "repository results"


def search_string(language):
    return LANGUAGE_SEARCH.format(language.replace(" ", "+"))


async def fetch(session, url):
    async with session.get(url) as response:
        return await response.text()


class Store:
    languages = []
    accounted = {}
    progress = 0
    should_sleep = False
    sleep_for = 0
    started = time.time()
    failed_results = []
    retries_remaining = 3


async def get_rate_limit(session):
    req = await session.get(GITHUB + "/rate_limit")
    res = await req.json()

    limit_remaining = res["resources"]["search"]["remaining"]
    limit_reset = res["resources"]["search"]["reset"]
    remaining_time = limit_reset - time.time()

    if int(limit_remaining) > 0:
        print(
            f"{limit_remaining} calls remaining, resets in "
            + time.strftime("%H:%M:%S", time.gmtime(remaining_time))
        )

    return limit_remaining, int(remaining_time) + 1


async def count_language(lang, store, session):
    url = search_string(lang)
    print(f"[{store.progress + 1}/{len(store.languages)}] {lang}")
    store.progress += 1

    req = await session.get(url)
    headers = req.headers
    limit_remaining = int(headers["X-RateLimit-Remaining"])
    limit_reset = int(headers["X-RateLimit-Reset"])
    remaining_time = limit_reset - time.time()

    res = await req.json()

    try:
        number = res["total_count"]
        print(f'{lang} - {str(number)}, incomplete - {res["incomplete_results"]}')

        if res["incomplete_results"]:
            store.failed_results.append(lang)

        store.accounted[lang] = number
    except KeyError:  # Probably hit the limit
        print(f"  FAILED - {lang}, KeyError")
        store.failed_results.append(lang)


async def initial_sweep(store, session):
    while store.progress != len(store.languages):
        limit, reset_in = await get_rate_limit(session)

        await asyncio.gather(
            *(
                asyncio.ensure_future(count_language(lang, store, session))
                for lang in store.languages[store.progress : store.progress + limit]
            )
        )

        if store.progress >= len(store.languages):
            break  # Done

        if limit <= 0:
            if reset_in > 0:
                print(f"\n  Limit reached, sleeping for {str(reset_in)} seconds…")
            else:
                print("  Waiting some more…")

            await asyncio.sleep(reset_in if reset_in > 0 else 2)


async def error_sweep(store, session):
    def pop_and_yield(count):
        if count > len(store.failed_results):
            count = len(store.failed_results)

        # Lang is gonna be readded, if it fails again
        while count > 0:
            count -= 1
            yield store.failed_results.pop()

    while store.retries_remaining > 0 and len(store.failed_results) > 0:
        print(f"{store.retries_remaining} retries remaining")

        limit, reset_in = await get_rate_limit(session)
        store.retries_remaining -= 1
        store.progress -= 1
        targets = []

        await asyncio.gather(
            *(
                asyncio.ensure_future(count_language(lang, store, session))
                for lang in pop_and_yield(limit)
            )
        )

        if store.retries_remaining <= 0 or len(store.failed_results) <= 0:
            break  # Done

        if limit <= 0:
            if reset_in > 0:
                print(f"\n  Limit reached, sleeping for {str(reset_in)} seconds…")
            else:
                print("  Waiting some more…")

            await asyncio.sleep(reset_in if reset_in > 0 else 2)


async def main():
    store = Store()
    headers = {"Authorization": "token " + TOKEN.read_text().strip()}

    with JSON_PATH.open("r") as file:
        languages_json = json.load(file)

        for key in languages_json.keys():
            if "color" in languages_json[key]:
                store.languages.append(key)

    conn = aiohttp.TCPConnector()
    session = aiohttp.ClientSession(connector=conn, headers=headers)

    # Initial sweep
    await initial_sweep(store, session)

    # Retry to resolve any incomplete / failed results
    print(f"Will now attempt to resolve {len(store.failed_results)} failed languages")
    await error_sweep(store, session)
    await session.close()

    summary = {
        "elapsed": time.strftime("%H:%M:%S", time.gmtime(time.time() - store.started)),
        "languages_targeted": len(store.languages),
        "languages_queried": len(store.accounted),
        "failed_count": len(store.failed_results),
        "failed_results": store.failed_results,
        "retries_remaining": store.retries_remaining,
    }

    COUNTED_JSON.write_text(json.dumps(store.accounted))
    SUMMARY_JSON.write_text(json.dumps(summary))

    print()
    print(f"Files written at: \n  {str(COUNTED_JSON)}\n  {str(SUMMARY_JSON)}")


def init():
    loop = asyncio.get_event_loop()

    loop.run_until_complete(main())
    loop.close()


init()
