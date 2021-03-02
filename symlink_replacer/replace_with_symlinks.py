from pathlib import Path
from os import symlink
import json
import sys


def main():
    try:
        with open('config.json') as json_file:
            cfg = json.loads(json_file.read())
            print(type(cfg))
            file_name = cfg["file_name"]
            folder = cfg["folder"]
    except ValueError as e:
        print('failure to read config.json')
        print(e)
        input("Press any key to exit")
        exit()

    if not folder:
        folder = Path(sys.argv[0])
        folder = folder.parents[1]
    else:
        folder = Path(folder)

    src = folder.joinpath(file_name)
    if not src.exists():
        # Todo: consider expanding this creating an empty file?
        print("No source symlink file present")
        exit()

    for file in folder.glob('**/' + file_name):
        if not file.is_symlink():
            file.unlink()
            symlink(src, file)


main()






