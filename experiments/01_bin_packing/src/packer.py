import numpy as np

class Shape:
    """
    Represents a 2D shape for bin packing.
    The shape is defined as a binary numpy array (0 = empty, 1 = filled).
    """
    def __init__(self, mask: np.ndarray):
        # Why: Ensure we are dealing with a 2D array representing a shape.
        # What: Initializes the Shape object.
        # Pre: mask must be a 2D numpy array of numeric type.
        # Post: A Shape object is created with a boolean mask and known width/height.
        assert isinstance(mask, np.ndarray), "mask must be a numpy ndarray"
        assert mask.ndim == 2, "mask must be a 2D array"

        # Store as boolean array for faster logic/overlap operations
        self.mask = mask.astype(bool)
        self.height, self.width = self.mask.shape

    @classmethod
    def create_rectangle(cls, width_mm: int, height_mm: int) -> 'Shape':
        """
        Helper to create a simple rectangular shape.
        """
        assert width_mm > 0, "width must be > 0"
        assert height_mm > 0, "height must be > 0"
        return cls(np.ones((height_mm, width_mm), dtype=bool))


class Bin:
    """
    Represents the rectangular field (the "Bin") we want to pack shapes into.
    Coordinates are (y, x), where y goes from 0 (bottom) to height (top),
    and x goes from 0 (left) to width (right).
    Internally represented as a 2D array where 0 = empty and 1 = filled.
    """
    def __init__(self, width_mm: int, height_mm: int):
        # Why: We need a discrete 2D space to track placed shapes.
        # What: Initializes the empty Bin of given dimensions.
        # Pre: width_mm > 0, height_mm > 0
        # Post: A Bin object is created with an empty boolean grid.
        assert width_mm > 0, "width_mm must be > 0"
        assert height_mm > 0, "height_mm must be > 0"

        self.width = width_mm
        self.height = height_mm
        self.grid = np.zeros((height_mm, width_mm), dtype=bool)

    def can_place(self, shape: Shape, x: int, y: int) -> bool:
        """
        Checks if a shape can be placed at the specified (x, y) coordinates
        (bottom-left corner of the shape's bounding box).
        """
        # Why: To ensure shape stays within bounds and does not overlap existing shapes.
        # What: Returns True if placement is valid, False otherwise.
        # Pre: shape must be a Shape object.
        # Post: Returns a boolean indicating placement feasibility.
        if x < 0 or y < 0:
            return False
        if x + shape.width > self.width or y + shape.height > self.height:
            return False

        # Extract the relevant subgrid
        subgrid = self.grid[y:y+shape.height, x:x+shape.width]

        # Check for overlap: bitwise AND between subgrid and shape mask.
        # If any element is True, there is an overlap.
        overlap = np.any(subgrid & shape.mask)
        return not overlap

    def place_shape(self, shape: Shape, x: int, y: int) -> None:
        """
        Mutates the bin's grid by placing the shape at (x, y).
        """
        # Why: We need to commit a valid placement to the bin's state.
        # What: Updates the grid with the shape's mask.
        # Pre: The placement must be valid (can_place returns True).
        # Post: The shape's pixels are written into the Bin grid.
        assert self.can_place(shape, x, y), "Cannot place shape at these coordinates (out of bounds or overlap)."

        subgrid = self.grid[y:y+shape.height, x:x+shape.width]
        # Use logical OR to place the shape onto the subgrid
        self.grid[y:y+shape.height, x:x+shape.width] = subgrid | shape.mask

    def pack_shape(self, shape: Shape) -> tuple[int, int]:
        """
        Attempts to pack a shape into the bin using a bottom-left first algorithm.
        It scans pixel-by-pixel, starting from y=0, x=0, and moving right, then up.
        """
        # Why: Finds the most optimal placement according to bottom-left packing heuristic.
        # What: Finds coordinates and places the shape.
        # Pre: shape is a valid Shape object.
        # Post: Returns (x, y) placement coordinates, or raises ValueError if it does not fit.

        for y in range(self.height - shape.height + 1):
            for x in range(self.width - shape.width + 1):
                if self.can_place(shape, x, y):
                    self.place_shape(shape, x, y)
                    return (x, y)

        raise ValueError("Shape does not fit in the bin.")
