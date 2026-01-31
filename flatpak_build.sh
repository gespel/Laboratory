#!/bin/bash
set -e

echo "Building Laboratory Flatpak..."

# Build and install the Flatpak
flatpak-builder --user --install --force-clean builddir packaging/de.philabs.laboratory.yml

echo ""
echo "✓ Flatpak build completed successfully!"
echo ""
echo "To run: flatpak run de.philabs.laboratory"
echo "To list: flatpak list --user | grep laboratory"
