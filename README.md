linkme
======

A utility that finds duplicate files and links them together


Design
======
The application accepts a number of directory names as command line arguments. These directories are recusivly traversed and any files with names that look like md5 checksums will be added to a binary tree. If the file already exists in the tree then instead of adding it a second time, the file is replaced with a link to the existing file in the tree.

For performance reasons the md5 checksums are converted to uint128 to avoid constly string comparisons. Also the binary tree is indexed by the first digit of the filenames (0-f), effectivly making them 16 separete binary trees.

For safty reasons, duplicate files are first moved to /tmp and if the link fails for some reason the original file is moved back.


