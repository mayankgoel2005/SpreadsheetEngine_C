import sys
import pandas as pd

def compare_excel_files(file1, file2):
    try:
        # Read both Excel files
        df1 = pd.read_excel(file1, sheet_name=None)  # Read all sheets
        df2 = pd.read_excel(file2, sheet_name=None)

        # Check if both have the same sheets
        if df1.keys() != df2.keys():
            print("Mismatch in sheet names")
            return 1

        # Compare each sheet
        for sheet in df1.keys():
            df1[sheet] = df1[sheet].replace('#DIV/0!', 'err')
            df2[sheet] = df2[sheet].replace('#DIV/0!', 'err')
            if not df1[sheet].equals(df2[sheet]):
                print(f"Difference found in sheet: {sheet}")
                return 1

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
