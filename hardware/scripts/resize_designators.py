import pcbnew


'''
Resize font size for all resistor and capacitor designators

To execute:
    In KiCad: Menu -> Tools -> Scripting Console ...
    Type in:
        >>> exec(open('./resize_designators.py', 'r').read())
'''


FONT_SIZE = 0.7
FOOTPRINTS = ('R', 'C')


# Load the current board
board = pcbnew.GetBoard()

# Set the desired font size
new_size = pcbnew.FromMM(FONT_SIZE)

# Create a VECTOR2I object for the new font size
new_size_vector = pcbnew.VECTOR2I(new_size, new_size)

# Iterate through all the footprints on the board
for module in board.GetFootprints():
    reference = module.GetReference()

    # Check if the footprint is a capacitor or resistor
    if reference.startswith(FOOTPRINTS):

        # Get the reference text object
        ref_text = module.Reference()

        # Set the new font size
        ref_text.SetTextSize(new_size_vector)

        # Get the value text object
        value_text = module.Value()

        # Hide the value
        value_text.SetVisible(False)

# Refresh the display
pcbnew.Refresh()
