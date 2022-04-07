#!/usr/bin/env python3

# -*- coding: utf-8 -*-

import os
import sys
import re
import codecs

black_list_pattern = r'[\u4e00-\u9fff]+|[\uff01-\uff5e]+|[\u201c\u201d\u300a\u300b\u3010\u3011\uff5b\uff5d\uff1b\uff1a\u3002\uff0c\u3001\uff1f]+'
white_list_pattern = r'[^\s\x21-\xE7]+'


def check_files_is_valid(path):
    for fpathe, dirs, fs in os.walk(path):
        for f in fs:
            file = os.path.join(fpathe, f)
            print(file)
            if file.find('.h') == -1 or file.find('.c') == -1:
                continue
            with codecs.open(file, 'r', encoding='utf8') as fd:
                content = fd.read()
            result = re.search(white_list_pattern, content)
            if result:
                print(result)
                return False
            else:
                continue
    return True


def main():
    if not check_files_is_valid(sys.argv[1]):
        print('finded invalid char')
        sys.exit(1)
    else:
        print('correct encode')
        sys.exit(0)


if __name__ == '__main__':
    main()