// Datos extraidos de TRNMOD.BIN y del ELF (ver training.h).
#include "training.h"
namespace vn {
const int8_t kTrnSeconds[16][3] = {{15,15,15}, {-1,-1,-1}, {9,11,14}, {55,45,35}, {15,15,15}, {15,15,15}, {-1,-1,-1}, {15,15,15}, {15,15,15}, {15,13,14}, {15,15,15}, {30,30,30}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}, {-1,-1,-1}};
const int32_t kTrnNormTrn01[3] = {13, 15, 17};
const int32_t kTrnNormTrn02[3] = {16, 16, 16};
const int32_t kTrnNormTrn03_04[3] = {45, 65, 85};
const int32_t kTrnNormTrn09[3] = {13, 13, 14};
const int32_t kTrn09Table[10] = {1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
const char* const kTrnBadWords[50] = {"FUC", "FUK", "ASS", "DIC", "DIK", "SHT", "SEX", "XX", "XXX", "TIT", "POO", "PLO", "AZZ", "KKK", "FU", "DOO", "NIP", "FCK", "PKK", "RAF", "ARS", "NIG", "JAP", "HUR", "PSS", "WNK", "ALK", "ETA", "PEO", "PCE", "ANO", "PIS", "CIU", "PNV", "UTA", "CAZ", "CUL", "VAF", "BIT", "PIP", "CON", "SUC", "MRD", "FN", "LSD", "GIA", "BUM", "CUM", "NOB", "IRA"};
const uint32_t kTrnLetterObj[26] = {0x6C000A, 0x6C000C, 0x6C0010, 0x6C0012, 0x6C0013, 0x6C0015, 0x6C0017, 0x6C0018, 0x6C0019, 0x6C001D, 0x6C001E, 0x6C001F, 0x6C0020, 0x6C0021, 0x6C002A, 0x6C002B, 0x6C002D, 0x6C002F, 0x6C0036, 0x6C0037, 0x6C0039, 0x6C003B, 0x6C003C, 0x6C003D, 0x6C003E, 0x6C0041};
const int kTrnTimeDigitTbl[5] = {0, 1, 3, 6, 8};
}
