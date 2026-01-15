#!/bin/bash
#
# build-in-docker-ubuntu22.sh
#
# This script builds the s9s-tools project inside a Docker container based on
# Ubuntu 22.04 and extracts the compiled binaries to a local directory.
#
# Usage: ./build-in-docker-ubuntu22.sh
#
# Output: The built binaries will be placed in the ./ubuntu22.04 directory
#

set -e  # Exit immediately if any command fails

# Configuration
IMAGE_NAME="s9s-tools-ubuntu22"
CONTAINER_NAME="s9s-tools-build-temp"
OUTPUT_DIR="ubuntu22.04"

echo "=========================================="
echo "Building s9s-tools on Ubuntu 22.04"
echo "=========================================="
echo ""

# Step 1: Build the Docker image
# This reads the Dockerfile.ubuntu22.04 in the current directory and builds an image
# containing the compiled s9s-tools binaries
echo "[1/5] Building Docker image '$IMAGE_NAME'..."
if docker build -f Dockerfile.ubuntu22.04 -t "$IMAGE_NAME" .; then
    echo "✓ Docker image built successfully"
else
    echo "✗ Failed to build Docker image"
    exit 1
fi
echo ""

# Step 2: Create a temporary container from the image
# We create (but don't start) a container so we can copy files from it
# The container is created from the image we just built
echo "[2/5] Creating temporary container '$CONTAINER_NAME'..."
if docker create --name "$CONTAINER_NAME" "$IMAGE_NAME"; then
    echo "✓ Container created"
else
    echo "✗ Failed to create container"
    exit 1
fi
echo ""

# Step 3: Copy the built binaries from the container to the host
# The binaries were placed in /build/ubuntu22.04 inside the container
# during the Docker build process. We copy them to the current directory.
echo "[3/5] Copying built binaries from container to ./$OUTPUT_DIR/..."
if docker cp "$CONTAINER_NAME:/build/$OUTPUT_DIR" ./; then
    echo "✓ Binaries copied successfully"
else
    echo "✗ Failed to copy binaries from container"
    docker rm "$CONTAINER_NAME" 2>/dev/null
    exit 1
fi
echo ""

# Step 4: Remove the temporary container
# Clean up the container since we no longer need it
echo "[4/5] Removing temporary container..."
if docker rm "$CONTAINER_NAME"; then
    echo "✓ Container removed"
else
    echo "✗ Failed to remove container"
    exit 1
fi
echo ""

# Step 5: Display the results
echo "[5/5] Build complete!"
echo ""
echo "=========================================="
echo "Built binaries are available in:"
echo "  ./$OUTPUT_DIR/"
echo ""
echo "Contents:"
ls -lh "$OUTPUT_DIR/"
echo ""
echo "=========================================="
echo ""
echo "You can now use the s9s binary:"
echo "  ./$OUTPUT_DIR/s9s --help"
echo ""
