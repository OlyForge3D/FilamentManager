Import("env")
import os
import re

def combine_html_files(source=None, target=None, env=None):
    print("COMBINE HTML FILES")
    
    html_dir = "./html"
    header_file = os.path.join(html_dir, "header.html")
    
    # Read header content
    with open(header_file, 'r') as f:
        header_content = f.read()
    
    # Process all HTML files except header.html
    for filename in os.listdir(html_dir):
        if filename.endswith('.html') and filename != 'header.html':
            file_path = os.path.join(html_dir, filename)
            
            # Read content
            with open(file_path, 'r') as f:
                content = f.read()
            
            # Replace content between head comments with header content
            pattern = r'(<!-- head -->).*?(<!-- head -->)'
            new_content = re.sub(pattern, r'\1' + header_content + r'\2', content, flags=re.DOTALL)
            
            # Write back combined content
            with open(file_path, 'w') as f:
                f.write(new_content)
            print(f"Combined header with {filename}")

# Hook the filesystem image itself, not the "buildfs" alias. An action on the
# alias runs only after the image has already been built from data/, so the
# combined HTML would not reach the image until the next build. Registering
# here also means firmware-only builds never rewrite html/, because the image
# target only exists when buildfs/uploadfs/uploadfsota is requested.
# gzip_files.py hooks the same target and is loaded after this script, so it
# runs second and picks up the combined output.
env.AddPreAction("$BUILD_DIR/${ESP32_FS_IMAGE_NAME}.bin", combine_html_files)
