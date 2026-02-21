import gzip
import os
import shutil

## gzip files

def compress_file(input_file, output_file):
    with open(input_file, 'rb') as f_in:
        with gzip.open(output_file, 'wb') as f_out:
            f_out.writelines(f_in)

def copy_file(input_file, output_file):
    shutil.copy2(input_file, output_file)

def should_compress(file):
     # Skip compression for spoolman.html
    if file == 'spoolman.html' or file == 'waage.html':
        return False
    # Only compress certain file types
    return file.endswith(('.js', '.png', '.css', '.html'))

def main(source_dir, target_dir):
    for root, dirs, files in os.walk(source_dir):
        rel_path = os.path.relpath(root, source_dir)
        for file in files:
            input_file = os.path.join(root, file)
            output_file_compressed = os.path.join(target_dir, rel_path, file + '.gz')
            output_file_original = os.path.join(target_dir, rel_path, file)
            
            os.makedirs(os.path.dirname(output_file_compressed), exist_ok=True)

            if should_compress(file):
                compress_file(input_file, output_file_compressed)
                print(f'Compressed {input_file} to {output_file_compressed}')
            else:
                copy_file(input_file, output_file_original)
                print(f'Copied {input_file} to {output_file_original}')

def init():
    source_dir = 'html'
    target_dir = 'data'
    
    if os.path.exists(target_dir):
        shutil.rmtree(target_dir)
    
    main(source_dir, target_dir)

init()