import os
import sys
import fallocate
import random

file_size = int(sys.argv[1]) * 1024 # 32 KB, 1 MB, 100 MB
# total_size = 1024 * 1024 * 1024 * 1 #5 GB total
# total_files = int(total_size / file_size)
total_files = 128

source_dir = 'test_src'
source_path = os.path.join(os.getcwd(), source_dir)

os.mkdir(source_path)

for i in range(5):
    os.mkdir(os.path.join(source_path, str(i)))

for i in range(total_files):
    dir_idx = random.randint(0,4)
    temp_path = os.path.join(os.path.join(source_dir, str(dir_idx)), str(i) + '.txt')
    with open(temp_path, 'w+') as f:
        fallocate.fallocate(f.fileno(), 0, file_size)
