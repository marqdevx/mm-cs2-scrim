#! vim: set ts=4 sw=4 tw=99 et:
import os
import json


def main():
    top_code = 0
    for name in os.listdir('manifests'):
        file = os.path.join('manifests', name)
        with open(file, 'rt') as fp:
            obj = json.load(fp)
        top_code = max(top_code, obj['code'])
    print(top_code + 1)


if __name__ == '__main__':
    main()
