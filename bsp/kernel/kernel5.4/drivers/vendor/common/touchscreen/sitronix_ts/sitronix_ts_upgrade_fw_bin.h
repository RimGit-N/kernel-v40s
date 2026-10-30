/*
*If you need to make an upgrade distinction,
*the following macro control only needs to be enabled one.
*/
/* #define ST_UPGRADE_BY_ISPID */
/* #define ST_UPGRADE_BY_FWID */

#define STX_UPGRADE_REQUEST_FW_FILE

/* #define ST_UPGRADE_VERSION_CTRL */

#ifdef ST_UPGRADE_BY_ISPID
#define SITRONIX_ISPIDS {\
0x0, 0x0, 0x0, 0x0, 0x3,\
0x0, 0x0, 0x0, 0x0, 0x3,\
0x01, 0x01, 0x02, 0x03, 0x04\
}
#elif defined(ST_UPGRADE_BY_FWID)
#define SITRONIX_IDS {\
0x0, 0x0, 0x0, 0x0,\
0x1, 0x2, 0x3, 0x4\
}
#define SITRONIX_ID_OFF {\
0xD6,\
0xD6\
}
#endif

/* hf  dump file */
#define SITRONIX_FW {\
}

#define SITRONIX_CFG {\
}

#define SITRONIX_FW1 {\
}

#define SITRONIX_CFG1 {\
}

#define SITRONIX_FW2 {\
}

#define SITRONIX_CFG2 {\
}
