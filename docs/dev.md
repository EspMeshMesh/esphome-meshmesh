# NanoPB

## After a change on a .proto file

```bash
cd componenets/meshmesh/proto
nanopb_generator -D ../ --strip-path nodeinfo.proto
cd ..
mv nodeinfo.pb.c nodeinfo.pb.cpp
cd ../../
```