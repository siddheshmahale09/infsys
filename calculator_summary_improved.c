#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Type definitions
typedef unsigned char UCHAR;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;

// Constants
#define DATE 1
#define ERRRBEEP 1
#define FH 1
#define DH 2
#define MAX_ASC_BUFFER 16
#define DATE_BUFFER_SIZE 2
#define BILLS_DIGITS 5
#define QTY_DIGITS 8
#define AMOUNT_DIGITS 12
#define QTY_DECIMAL_POS 5
#define AMOUNT_DECIMAL_POS 10

// Status constants
#define STATUS_COMPLETE 'C'
#define FINAL_RECORD 1

// Delay constants
#define LOOP_DISPLAY_DELAY 200
#define MATCH_DISPLAY_DELAY 1000
#define NO_MATCH_DISPLAY_DELAY 500
#define ERROR_DISPLAY_DELAY 2000
#define NO_RECORDS_DELAY 2500

// Global variables (assumed from original code)
extern UCHAR temp_dd, temp_mm, temp_yy;
extern UCHAR double_height;
extern UCHAR asc[MAX_ASC_BUFFER];
extern UINT64 value_hex_asc_64U;

// Structure definitions
struct mc_struct {
    struct {
        UINT32 recs;
    } mc;
} mc;

struct rec_struct {
    struct {
        UCHAR bill_stat;
        UCHAR calc_stat;
        UCHAR last;
        UCHAR dd[DATE_BUFFER_SIZE];
        UCHAR mm[DATE_BUFFER_SIZE];
        UCHAR yy[DATE_BUFFER_SIZE];
        UINT32 quantity;
        UINT64 round_off_tot_amt;
    } rec;
} rec;

// Summary data structure
typedef struct {
    UINT32 totalBills;
    UINT32 totalQty;
    UINT64 totalAmount;
} SummaryData;

// Date range structure
typedef struct {
    UINT32 fromMins;
    UINT32 toMins;
} DateRange;

// Function prototypes
void read_rtc(void);
void tim_hex_asc(void);
int date_entry(const char* prompt, int type);
UINT32 get_mins(UCHAR dd, UCHAR mm, UCHAR yy, UCHAR h, UCHAR m, UCHAR s);
void clrscr(void);
void gotoxy(int x, int y);
void lcdprint(const char* str);
void beep(int type);
void Delay_ms(int ms);
void get_rec_report(UINT32 loop, int pg);
void hex_asc(UINT32 value, UCHAR* buffer, int digits);
void disp_char(UCHAR c);
void pk_unpk(UCHAR* packed, UCHAR* unpacked);
UINT32 asc_hex(UCHAR* buffer, int len);
void disp_printmsg(void);
void print_head(int type, int flag);
void unicode_prt_buf(const char* str);
void unicode_print(int type);
void prt_bar(void);
void print_date_time(void);
void rem_zeros(UCHAR* buffer, int min_digits);
void unicode_prnbuf(UCHAR c);
void hex_asc_64U(UCHAR* buffer, int digits);
void paper_feed(void);

// Improved helper functions
static void showErrorMessage(const char* message, int delayMs);
static bool getDateRange(DateRange* range);
static bool isValidRecord(void);
static bool getRecordDate(UINT32* recordMins);
static void displayProcessingInfo(UINT32 loop);
static void updateSummaryData(SummaryData* summary);
static void printSummaryReport(const SummaryData* summary);
static void printFormattedNumber(UINT32 value, int digits, int decimalPos, bool hasDecimal);
static void printFormattedAmount(UINT64 value);

/**
 * Show error message on LCD with beep and delay
 */
static void showErrorMessage(const char* message, int delayMs)
{
    clrscr();
    double_height = 1;
    gotoxy(3, 1);
    lcdprint(message);
    beep(ERRRBEEP);
    Delay_ms(delayMs);
}

/**
 * Get date range from user input
 * Returns true if successful, false if user cancelled
 */
static bool getDateRange(DateRange* range)
{
    UCHAR fromDD, fromMM, fromYY;
    UCHAR toDD, toMM, toYY;

    // Get FROM date
    if (!date_entry("   ENTER FROM DATE  ", DATE)) {
        return false;
    }
    fromDD = temp_dd;
    fromMM = temp_mm;
    fromYY = temp_yy;

    // Get TO date
    if (!date_entry("    ENTER TO DATE   ", DATE)) {
        return false;
    }
    toDD = temp_dd;
    toMM = temp_mm;
    toYY = temp_yy;

    // Convert to minutes for comparison
    range->fromMins = get_mins(fromDD, fromMM, fromYY, 0, 0, 0);
    range->toMins = get_mins(toDD, toMM, toYY, 0, 0, 0);

    // Validate date range
    if (range->fromMins > range->toMins) {
        showErrorMessage(" INVALID DATE RANGE ", ERROR_DISPLAY_DELAY);
        return false;
    }

    return true;
}

/**
 * Check if current record meets all criteria
 */
static bool isValidRecord(void)
{
    return (rec.rec.bill_stat == STATUS_COMPLETE) &&
           (rec.rec.calc_stat == STATUS_COMPLETE) &&
           (rec.rec.last == FINAL_RECORD);
}

/**
 * Extract and convert record date to minutes
 * Returns true if successful, false on error
 */
static bool getRecordDate(UINT32* recordMins)
{
    UCHAR dd2[DATE_BUFFER_SIZE], mm2[DATE_BUFFER_SIZE], yy2[DATE_BUFFER_SIZE];
    
    // Unpack date fields
    pk_unpk(rec.rec.dd, dd2);
    pk_unpk(rec.rec.mm, mm2);
    pk_unpk(rec.rec.yy, yy2);

    // Convert to integers
    UINT32 recDD = asc_hex(dd2, DATE_BUFFER_SIZE);
    UINT32 recMM = asc_hex(mm2, DATE_BUFFER_SIZE);
    UINT32 recYY = asc_hex(yy2, DATE_BUFFER_SIZE);

    // Basic date validation
    if (recDD == 0 || recDD > 31 || recMM == 0 || recMM > 12) {
        return false;
    }

    // Display date for debugging
    gotoxy(3, 1);
    lcdprint("D:");
    hex_asc(recDD, asc, 2); lcdprint(asc);
    hex_asc(recMM, asc, 2); lcdprint(asc);
    hex_asc(recYY, asc, 2); lcdprint(asc);

    *recordMins = get_mins(recDD, recMM, recYY, 0, 0, 0);
    return true;
}

/**
 * Display current processing information
 */
static void displayProcessingInfo(UINT32 loop)
{
    gotoxy(1, 1);
    lcdprint("LOOP:");
    hex_asc(loop, asc, 3);
    lcdprint(asc);
    Delay_ms(LOOP_DISPLAY_DELAY);

    gotoxy(2, 1);
    lcdprint("B:");
    disp_char(rec.rec.bill_stat);
    lcdprint(" C:");
    disp_char(rec.rec.calc_stat);
    lcdprint(" L:");
    disp_char(rec.rec.last + '0');
}

/**
 * Update summary data with current record
 */
static void updateSummaryData(SummaryData* summary)
{
    summary->totalBills++;
    summary->totalQty += rec.rec.quantity;
    summary->totalAmount += rec.rec.round_off_tot_amt;
    
    gotoxy(4, 1);
    lcdprint("MATCHED++");
}

/**
 * Print formatted number with optional decimal point
 */
static void printFormattedNumber(UINT32 value, int digits, int decimalPos, bool hasDecimal)
{
    UCHAR i;
    
    hex_asc(value, asc, digits);
    rem_zeros(asc, 4);
    
    for (i = 0; i < digits; i++) {
        if (hasDecimal && i == decimalPos) {
            unicode_prnbuf('.');
        }
        unicode_prnbuf(asc[i]);
    }
}

/**
 * Print formatted 64-bit amount
 */
static void printFormattedAmount(UINT64 value)
{
    UCHAR i;
    
    value_hex_asc_64U = value;
    hex_asc_64U(asc, AMOUNT_DIGITS);
    rem_zeros(asc, 8);
    
    for (i = 0; i < AMOUNT_DIGITS; i++) {
        if (i == AMOUNT_DECIMAL_POS) {
            unicode_prnbuf('.');
        }
        unicode_prnbuf(asc[i]);
    }
}

/**
 * Print the summary report
 */
static void printSummaryReport(const SummaryData* summary)
{
    disp_printmsg();
    print_head(1, 0);
    unicode_prt_buf("         CALCULATOR SUMMARY REPORT         ");
    unicode_print(FH);
    prt_bar();
    print_date_time();
    prt_bar();
    unicode_prt_buf("BILLS           QUANTITY             AMOUNT");
    unicode_print(DH);
    prt_bar();

    // Print Bills count
    printFormattedNumber(summary->totalBills, BILLS_DIGITS, 0, false);
    unicode_prt_buf("           ");

    // Print Quantity with decimal
    printFormattedNumber(summary->totalQty, QTY_DIGITS, QTY_DECIMAL_POS, true);
    unicode_prt_buf("          ");

    // Print Amount with decimal
    printFormattedAmount(summary->totalAmount);
    unicode_print(DH);
    prt_bar();
    print_head(0, 0);
    paper_feed();
}

/**
 * Improved Calculator Summary Report Function
 * 
 * Improvements made:
 * 1. Added bounds checking and validation
 * 2. Separated concerns into helper functions
 * 3. Added named constants for magic numbers
 * 4. Improved error handling
 * 5. Reduced nesting complexity
 * 6. Added comprehensive documentation
 * 7. Used structured data types
 * 8. Added input validation
 */
void calculatorSummaryReportImproved(void)
{
    UINT32 loop, recs;
    SummaryData summary = {0, 0, 0}; // Initialize all fields to 0
    DateRange dateRange;

    // Initialize system
    read_rtc();
    tim_hex_asc();

    // Get date range from user
    if (!getDateRange(&dateRange)) {
        return; // User cancelled or invalid range
    }

    // Get total number of records
    recs = mc.mc.recs;
    
    // Validate record count
    if (recs <= 1) {
        showErrorMessage(" NO RECORDS FOUND!!", NO_RECORDS_DELAY);
        return;
    }

    // Process all records
    for (loop = 1; loop < recs; loop++) {
        // Read current record
        get_rec_report(loop, 1);
        
        // Display processing information
        displayProcessingInfo(loop);

        // Check if record meets criteria
        if (!isValidRecord()) {
            gotoxy(4, 1);
            lcdprint("NOT MATCH");
            Delay_ms(NO_MATCH_DISPLAY_DELAY);
            continue;
        }

        // Get and validate record date
        UINT32 recordMins;
        if (!getRecordDate(&recordMins)) {
            gotoxy(4, 1);
            lcdprint("DATE ERROR");
            Delay_ms(NO_MATCH_DISPLAY_DELAY);
            continue;
        }

        // Check if record is within date range
        if (recordMins >= dateRange.fromMins && recordMins <= dateRange.toMins) {
            updateSummaryData(&summary);
            Delay_ms(MATCH_DISPLAY_DELAY);
        } else {
            gotoxy(4, 1);
            lcdprint("DATE SKIP");
            Delay_ms(MATCH_DISPLAY_DELAY);
        }
    }

    // Print results
    if (summary.totalBills > 0) {
        printSummaryReport(&summary);
    } else {
        showErrorMessage(" NO RECORDS FOUND!!", NO_RECORDS_DELAY);
    }
}

// Wrapper to maintain backward compatibility
void calculatorSummaryReport(void)
{
    calculatorSummaryReportImproved();
}

int main(void)
{
    printf("Improved Calculator Summary Report Function\n");
    printf("==========================================\n\n");
    
    printf("Key Improvements Made:\n");
    printf("1. ✅ Added bounds checking and input validation\n");
    printf("2. ✅ Defined named constants for all magic numbers\n");
    printf("3. ✅ Separated concerns into focused helper functions\n");
    printf("4. ✅ Enhanced error handling with meaningful messages\n");
    printf("5. ✅ Reduced nesting complexity for better readability\n");
    printf("6. ✅ Added comprehensive documentation\n");
    printf("7. ✅ Used structured data types for better organization\n");
    printf("8. ✅ Maintained backward compatibility\n\n");
    
    printf("Function Structure:\n");
    printf("- Main function: calculatorSummaryReportImproved()\n");
    printf("- Helper functions for specific tasks\n");
    printf("- Clear separation of UI, logic, and printing\n");
    printf("- Consistent error handling patterns\n");
    
    return 0;
}