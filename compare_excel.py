import sys
import pandas as pd
import numpy as np

def compare_excel_files(file1, file2):
    try:
        # Read both Excel files (all sheets)
        df1 = pd.read_excel(file1, sheet_name=None)
        df2 = pd.read_excel(file2, sheet_name=None)

        # Check if both have the same sheets
        if df1.keys() != df2.keys():
            print("Mismatch in sheet names")
            return 1

        # Compare each sheet
        for sheet in df1.keys():
            # Replace #DIV/0! with "err" in both dataframes
            df1[sheet] = df1[sheet].replace('#DIV/0!', 'err')
            df2[sheet] = df2[sheet].replace('#DIV/0!', 'err')

            # If the shapes differ, there's an immediate mismatch
            if df1[sheet].shape != df2[sheet].shape:
                print(f"Difference found in sheet '{sheet}': different dimensions")
                return 1

            # Compare the two dataframes cell-by-cell
            diff = df1[sheet].ne(df2[sheet])  # DataFrame of booleans (True where they differ)

            if diff.any().any():
                # At least one cell differs. Let's print each mismatched cell.
                mismatch_positions = np.where(diff)  # Returns a tuple of (row_indices, col_indices)
                for row, col in zip(*mismatch_positions):
                    val1 = df1[sheet].iloc[row, col]
                    val2 = df2[sheet].iloc[row, col]
                    # Print 1-based row/column to mimic Excel notation
                    print(f"Difference found in sheet '{sheet}' at cell "
                          f"(row={row+1}, col={col+1}): '{val1}' vs '{val2}'")
                return 1

        # If no differences found in any sheet
        print("Excel files are identical")
        return 0

    except Exception as e:
        print(f"Error: {e}")
        return 1

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_excel.py <file1.xlsx> <file2.xlsx>")
        sys.exit(1)

    file1, file2 = sys.argv[1], sys.argv[2]
    result = compare_excel_files(file1, file2)
    sys.exit(result)