import sys
import os
import pytest
import numpy as np

# Add src directory to path to allow importing packer
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../src')))

from packer import Bin, Shape

def test_shape_initialization():
    """Test creation of rectangular shapes."""
    s = Shape.create_rectangle(10, 5)
    assert s.width == 10
    assert s.height == 5
    assert s.mask.shape == (5, 10)
    assert np.all(s.mask == True)

def test_bin_initialization():
    """Test creation of the bin grid."""
    b = Bin(100, 50)
    assert b.width == 100
    assert b.height == 50
    assert b.grid.shape == (50, 100)
    assert np.all(b.grid == False)

def test_pack_single_shape():
    """Test packing a single shape into an empty bin."""
    b = Bin(100, 100)
    s = Shape.create_rectangle(20, 10)

    # Should be placed at (0, 0)
    x, y = b.pack_shape(s)
    assert x == 0
    assert y == 0

    # Verify grid state
    assert np.all(b.grid[0:10, 0:20] == True)
    assert np.sum(b.grid) == 200 # Area of the rectangle

def test_pack_multiple_shapes():
    """Test packing multiple shapes without overlap."""
    b = Bin(50, 50)

    s1 = Shape.create_rectangle(20, 20)
    x1, y1 = b.pack_shape(s1)
    assert (x1, y1) == (0, 0)

    s2 = Shape.create_rectangle(20, 20)
    # Bottom-left packing: s1 occupies (0,0) to (20,20).
    # Scanning y=0..50, x=0..50.
    # At y=0, x=20, s2 fits.
    x2, y2 = b.pack_shape(s2)
    assert (x2, y2) == (20, 0)

    s3 = Shape.create_rectangle(15, 20)
    # At y=0, x=40: width left is 10, s3 width is 15 -> doesn't fit.
    # At y=20, x=0: s3 fits.
    x3, y3 = b.pack_shape(s3)
    assert (x3, y3) == (0, 20)

    # Total area occupied: 400 + 400 + 300 = 1100
    assert np.sum(b.grid) == 1100

def test_pack_collision_and_bounds():
    """Test shapes hitting the bounds and overlapping."""
    b = Bin(30, 20)

    s1 = Shape.create_rectangle(20, 20)
    x1, y1 = b.pack_shape(s1)
    assert (x1, y1) == (0, 0)

    s2 = Shape.create_rectangle(15, 10)
    # Remaining width is 10, height is 20.
    # The shape s2 needs 15 width. It will fail to pack.
    with pytest.raises(ValueError, match="Shape does not fit in the bin."):
        b.pack_shape(s2)

def test_custom_shape_with_holes():
    """Test packing a custom shape (like an L-shape) and utilizing holes."""
    # Use a small bin so the bottom-left algorithm doesn't just put it next to the shape
    b = Bin(3, 3)

    # Create an L-shape
    # 1 0 0
    # 1 0 0
    # 1 1 1
    mask = np.array([
        [1, 1, 1],
        [1, 0, 0],
        [1, 0, 0]
    ])
    s_l = Shape(mask)
    x1, y1 = b.pack_shape(s_l)
    assert (x1, y1) == (0, 0)

    # We should be able to pack a 1x1 block into the "hole" at (1, 1)
    s_small = Shape.create_rectangle(1, 1)
    x2, y2 = b.pack_shape(s_small)
    assert (x2, y2) == (1, 1)
