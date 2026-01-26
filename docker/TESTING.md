# Docker Image Format Testing Guide

## Overview
This document describes how to test the kimageformats-based Docker image for YACReaderLibraryServer with AVIF and JXL support.

## Prerequisites
- Docker installed
- Test comics with various image formats available at `/exos/comics/Mylar/_Incoming_Downloads/Format testing/`
- Files needed: `avif.cbz`, `jxl.cbz`, `test_combined.cbz`

## Build the Image

```bash
cd /home/nick/yacreader
docker build -f docker/Dockerfile -t yacreader-test:kimageformats .
```

Expected: Build completes successfully with 6 stages (base, plugin-base, kimageformats-builder, sevenzip-builder, yacreader-builder, runtime)

## Copy Test Files

```bash
mkdir -p /home/nick/yacreader/test-comics
cp "/exos/comics/Mylar/_Incoming_Downloads/Format testing/"*.cbz /home/nick/yacreader/test-comics/
ls -la /home/nick/yacreader/test-comics/
```

Expected: See avif.cbz, jxl.cbz, test_combined.cbz, test_heic.cbz, test_heif.cbz

## Run Container

```bash
docker rm -f YACReaderTest 2>/dev/null
docker run --rm -d \
  --name YACReaderTest \
  -p 8086:8080 \
  -v /home/nick/yacreader/test-comics:/comics \
  -e PUID=$(id -u) \
  -e PGID=$(id -g) \
  yacreader-test:kimageformats

# Wait for startup
sleep 5
```

## Verify Image Format Support

```bash
docker logs YACReaderTest 2>&1 | grep "Image formats"
```

Expected output should include:
- `avif` ✅
- `avifs` ✅
- `jxl` ✅
- Plus bonus formats: ani, hdr, pcx, psd, qoi, xcf, etc.

**Should NOT include:** heic, heif (not supported in kimageformats 6.0.0)

## Create and Scan Library

```bash
# Create .yacreaderlibrary directory with correct permissions
docker exec -u abc YACReaderTest mkdir -p /comics/.yacreaderlibrary

# Create library and scan comics
docker exec YACReaderTest YACReaderLibraryServer create-library "TestComics" /comics
```

Expected output:
```
Processing comics...Done!
Number of comics processed = 3
```

Note: Only 3 comics should be processed (avif.cbz, jxl.cbz, test_combined.cbz). The HEIC/HEIF files are skipped because format not supported.

## Verify Comics via Web Interface

```bash
curl -s http://localhost:8086/library/1/folder/1 | grep -i "comic" | head -15
```

Expected: HTML output showing 3 comics with sizes and page counts

## Check Specific Comic Details

```bash
docker exec YACReaderTest ls -la /comics/.yacreaderlibrary/
```

Expected:
- `library.ydb` (SQLite database)
- `covers/` directory
- `id` file

## Verify Plugins Are Installed

```bash
docker exec YACReaderTest ls -la /usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats/ | grep kimg
```

Expected output should include:
- `kimg_avif.so` ✅
- `kimg_jxl.so` ✅
- Plus: kimg_ani.so, kimg_hdr.so, kimg_pcx.so, kimg_psd.so, kimg_qoi.so, kimg_xcf.so, etc.

## Cleanup

```bash
docker stop YACReaderTest
docker rm YACReaderTest 2>/dev/null
```

## Known Limitations

1. **HEIC/HEIF not supported**: kimageformats 6.0.0 (compatible with Ubuntu Noble's Qt 6.4.2) doesn't include HEIC/HEIF plugins. Test files `test_heic.cbz` and `test_heif.cbz` will be skipped during library scan.

2. **Compatibility patch required**: kimageformats 6.0.0 requires Qt 6.5+, but Ubuntu Noble has Qt 6.4.2. The Dockerfile includes a sed command to patch the version check: `sed -i 's/6\.5\.0/6.4.0/g' CMakeLists.txt`

## Success Criteria

- [x] Build completes without errors
- [x] Container starts and shows AVIF/JXL in supported formats
- [x] Library creation processes 3 comics successfully
- [x] Web interface shows all 3 comics
- [x] kimageformats plugins are installed in correct location
- [x] HEIC/HEIF files are gracefully skipped (not errors)

## Comparison Test (Optional)

Compare with the original PR #499 implementation:

```bash
# Original (PR #499)
docker logs YACReaderLibraryServer 2>&1 | grep "Image formats"
# Should show: avif, avifs, heic, heif, jxl

# New (kimageformats)
docker logs YACReaderTest 2>&1 | grep "Image formats"
# Should show: avif, avifs, jxl (+ 30+ bonus formats, no heic/heif)
```

## Troubleshooting

**Problem:** "Unable to find database at: /comics/.yacreaderlibrary"
- **Solution:** Ensure .yacreaderlibrary directory exists with correct permissions. Use `create-library` instead of `add-library` + `update-library`.

**Problem:** No comics scanned
- **Solution:** Check file permissions. Container runs as uid/gid 1000 (abc user). Files should be readable by this user.

**Problem:** kimageformats plugins not found
- **Solution:** Verify COPY command in Dockerfile uses `*.so` wildcard and paths match build output structure.
