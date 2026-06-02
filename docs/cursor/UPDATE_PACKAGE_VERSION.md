# Procedure to update and publish new espmeshmesh library dependency

## Procedure

1. Change components/meshmesh/__init__.py 
    - Be sure that a line with cg.add_library("ESPMeshMesh", "1.2.3") exists, is uncommented, and points to the requested espmeshmesh version
    - Be sure the cg.add_library lines that point to a local file path are commented
2. Commit the changes to this file with the message: update to library version 1.2.3
3. Push changes to the origin repository


