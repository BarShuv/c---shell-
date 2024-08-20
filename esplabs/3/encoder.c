#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char **argv) {

    int c;
    int encoding_direction = 1;  // 1 for addition, -1 for subtraction
    int key_length = 0;
    int encoding_index = 0;
    char* debug_mode = "on"; // Initialize debug mode to "on" by default
    int e_index = -1;
    int original_c = 0;
    char *key_index = NULL;
    char *input_filename = NULL;
    int offset = 0;

    // File pointers for input and output
    FILE *infile = stdin;  // Default input from stdin
    FILE *outfile = stdout;  // Default output to stdout


    // Loop through command-line arguments
    for (int i = 1; i < argc; i++) {
        // Check if debug mode change is needed
        if (strcmp(debug_mode, "on") == 0) {
            fprintf(stderr, "%s\n", argv[i]);
        }
        if (strcmp(argv[i], "-D") == 0) {
            debug_mode = "off";
        } else if (strcmp(argv[i], "+D") == 0) {
            debug_mode = "on";
        }

        // Check if argument starts with "+e" or "-e"
        if ((strncmp(argv[i], "+e", 2) == 0 || strncmp(argv[i], "-e", 2) == 0)) {
            // Determine encoding direction
            e_index = i;
            if (argv[i][0] == '-') {
                encoding_direction = -1;
            }
            // Extract key length
            key_index = argv[i] + 2;
            while (*key_index != '\0') {
                key_length++;
                key_index++;
            }
        }

        // Check for input file setting
        if (strncmp(argv[i], "-I", 2) == 0) {
            input_filename = argv[i] + 2;
            infile = fopen(input_filename, "r");
            if (infile == NULL) {
                fprintf(stderr, "Error: Unable to open input file %s\n", input_filename);
                return 1;
            }
        }

        // Check for output file setting
        if (strncmp(argv[i], "-O", 2) == 0) {
            char *output_filename = argv[i] + 2;
            outfile = fopen(output_filename, "w");
            if (outfile == NULL) {
                fprintf(stderr, "Error: Unable to open output file %s\n", output_filename);
                return 1;
            }
        }
    }

    // Read characters from stdin and write to stdout
    while (!feof(infile)) {
        c = fgetc(infile);
        if (c == EOF) {
            break;
        }
        // Check if character is a lowercase letter or digit
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            offset = argv[e_index][encoding_index+2] - '0'; // Convert char to int
            // Adjust offset for subtraction
            if (encoding_direction == -1)
                offset *= -1;
            original_c = c;
            c += offset;
            if (original_c >= 'a' && original_c <= 'z') {
                if (c < 'a') c += 26; // Wrap around
                if (c > 'z') c -= 26;
            }
            else if (original_c >= 'A' && original_c <= 'Z') {
                if (c < 'A') c += 26; // Wrap around
                if (c > 'Z') c -= 26;
            }
            else if (original_c >= '0' && original_c <= '9') {
                if (c < '0') c += 10; // Wrap around
                if (c > '9') c -= 10;
            }
        }
        // Move to the next digit in the key, wrapping around if necessary
        encoding_index = ((encoding_index + 1)) % key_length;
        fputc(c, outfile);

    }
    // Close files and exit
    fclose(infile);
    fclose(outfile);
    return 0;
}