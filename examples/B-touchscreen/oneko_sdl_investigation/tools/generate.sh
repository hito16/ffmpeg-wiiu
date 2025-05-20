#!/bin/bash

NEKO_XBM_H=neko_xbm.h
XBM_IMAGE_H=xbm_images.h

create_neko_xbm_h() {
  find ../oneko-master/bit*/neko -name "*.xbm" -print0 |
    while IFS= read -r -d $'\0' file; do
      dirname=$(dirname "$file")
      filename=$(basename "$file")
      printf "/* %s %s */\\n" " $dirname" " $filename"
      cat "$file"
    done > $NEKO_XBM_H
}

create_paired_xbm_list() {
  grep "static char" neko_xbm.h | sed 's/static char //g; s/\[.*//g' | sort | grep -v space_mask_bits |
    while IFS= read -r line1 && IFS= read -r line2; do
      echo "$line1 $line2"
    done
}

generate_xbm_images_data() {
  output_c="
// Array of XBM data structures from neko_xbm.h
// Define a struct to hold both bitmap and mask data
typedef struct {
    const unsigned char* bits;
    const unsigned char* mask_bits;
    int width;
    int height;
} XbmImageData;

XbmImageData xbm_images[] = {
"

  # Process each line of the input data
  while IFS= read -r line; do
    # Split the line into two variables: bits_name and mask_bits_name
    read -r bits_name mask_bits_name <<<"$line"

    # Extract the base name (remove "_bits")
    base_name=$(echo "$bits_name" | sed 's/_bits$//')

    # Append to the output string
    output_c+="    { (const unsigned char*)$bits_name, (const unsigned char*)$mask_bits_name, ${base_name}_width, ${base_name}_height },
"
  done

  # Complete the C code
  output_c+="};
"

  # Print the generated C code
  echo "$output_c" > $XBM_IMAGE_H

}

create_neko_xbm_h
create_paired_xbm_list | generate_xbm_images_data
