import pcbnew

# Load the current board
board = pcbnew.GetBoard()

# Set the desired font size
new_size = pcbnew.FromMM(0.7)

# Create a VECTOR2I object for the new font size
new_size_vector = pcbnew.VECTOR2I(new_size, new_size)

# Iterate through all the footprints on the board
for module in board.GetFootprints():
    reference = module.GetReference()

    # Check if the footprint is a capacitor or resistor
    if reference.startswith("C") or reference.startswith("R"):

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
