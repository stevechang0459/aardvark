#ifndef NVME_MI_H
#define NVME_MI_H

#include "types.h"
#include <stdbool.h>

/**
 * DSP0235, 7.6 Maximum message size
 *
 * The MCTP message body (including IC bit, Message Type, Message type-specific
 * header fields, message payload and message integrity check if present) for
 * NVMe Management Messages over MCTP shall be less than or equal to 4224
 * (4K+128) bytes.
 *
 * ---
 * NVMe-MI, 3.1 NVMe-MI Messages
 *
 * In the out-of-band mechanism, an NVMe-MI Message consists of the payload of
 * one or more MCTP packets. The maximum sized NVMe-MI Message is 4,224 bytes
 * (i.e., 4 KiB + 128 bytes). Refer to the NVMe Management Messages over MCTP
 * Binding Specification.
 */
#define NVME_MI_MSG_SIZE                (4096 + 128)

// NVMe-MI Message Type (NMIMT)
enum nvme_mi_msg_type {
	NMIMT_CTRLP = 0,                    // Control Primitive
	NMIMT_MI = 1,                       // NVMe-MI Command
	NMIMT_ADMIN,                        // NVMe Admin Command
	NMIMT_RSVD,                         // Reserved
	NMIMT_PCIE,                         // PCIe Command
	// 5h to Fh: Reserved
	NMIMT_MAX,
};

enum nvme_mi_msg_ror {
	ROR_REQ = 0,
	ROR_RESP = 1,
	ROR_MAX,
};

#pragma pack(push)
#pragma pack(1)

// NVMe-MI Message Header (NMH)
union nvme_mi_msg_header {
	struct {
		dword mt    : 7;        // Bit[6:0]   Message Type (MT)
		dword ic    : 1;        // Bit[7]     Integrity Check (IC)
		dword csi   : 1;        // Bit[8]     Command Slot Identifier (CSI)
		dword rsvd1 : 2;        // Bit[10:9]  Reserved
		dword nmimt : 4;        // Bit[14:11] NVMe-MI Message Type (NMIMT)
		dword ror   : 1;        // Bit[15:15] Request or Response (ROR)
		dword meb   : 1;        // Bit[16:16] Management Endpoint Buffer (MEB)
		dword ciap  : 1;        // Bit[17:17] Command Initiated Auto Pause (CIAP)
		dword rsvd2 : 14;       // Bit[31:18] Reserved
	};
	dword value;
};

/**
 * NVMe Management Dword 0 (NMD0) - Command Dword 12 (CDW12)
 */
union nvme_mi_nmd0 {
	// struct nmd0_rnmds rnmds;         // Read NVMe-MI Data Structure – NVMe Management Dword 0
	// struct nmd0_chsp chsp;           // Controller Health Status Poll – NVMe Management Dword 0
	// union nmd0_config cfg;           // Configuration Set/Get – NVMe Management Dword 0
	// struct nmd0_vpdr vpdr;           // VPD Read – NVMe Management Dword 0
	// struct nmd0_rst rst;             // Reset – NVMe Management Dword 0
	// struct nmd0_sh sh;               // Shutdown – NVMe Management Dword 0
	dword value;
};

/**
 * NVMe Management Dword 1 (NMD1) - Command Dword 13 (CDW13)
 */
union nvme_mi_nmd1 {
	// struct nmd1_rnmds rnmds;            // Read NVMe-MI Data Structure
	// struct nmd1_nshsp nshsp;            // NVM Subsystem Health Status Poll
	// struct nmd1_chsp chsp;              // Controller Health Status Poll
	// union nmd1_config cfg;              // Configuration Set/Get
	// struct nmd1_vpdr vpdr;              // VPD Read
	dword value;
};

// Invalid Parameter Error Response
struct nvme_mi_resp_invld_para {

	dword status : 8;                   // Bit[7:0] Response Message Status
	/**
	 * Bit[10:8]
	 * Least-significant bit in the least-significant uint8_t of the Request
	 * Message of the parameter that contained the error. Valid values are 0 to
	 * 7.
	 */
	dword lsbit : 3;
	// Bit[15:11] Reserved
	dword rsvd : 5;
	/**
	 * Bit[31:16]
	 * Least-significant uint8_t of the Request Message of the parameter that
	 * contained the error. If the error is beyond uint8_t 65,535, then the value
	 * 65,535 is reported in this field.
	 */
	dword lsbyte : 16;
};

/**
 * NVMe-MI Command Request Message Format (without Request Data and MIC)
 */
union nvme_mi_req_dw {
	struct {
		union nvme_mi_msg_header nmh;   // Command Dword 10 (CDW10) - NVMe-MI Message Header (NMH)
		uint8_t opc;                       // Command Dword 11 (CDW11) - Opcode (OPC)
		uint8_t rsvd[3];
		union nvme_mi_nmd0 nmd0;        // Command Dword 12 (CDW12) - NVMe Management Dword 0 (NMD0)
		union nvme_mi_nmd1 nmd1;        // Command Dword 13 (CDW13) - NVMe Management Dword 1 (NMD1)
	};
	uint8_t raw_data[16];
};

/**
 * NVMe Management Response (NMRESP) and Status (STATUS)
 */
union nvme_mi_resp {
	struct {
		dword status : 8;       // bit[7:0] Tunneled Status (STATUS)
		dword nmresp : 24;      // bit[31:8] NVMe Management Response (NMRESP)
	};
	// struct nvme_mi_resp_rnmds rnmds;             // NVMe Management Response - Read NVMe-MI Data Structure
	// struct nvme_mi_resp_chsp chsp;                       // NVMe Management Response - Controller Health Status Poll
	// struct nvme_mi_resp_error error;             // NVMe Management Response - Generic Error Response
	struct nvme_mi_resp_invld_para invld_para;      // NVMe Management Response - Invalid Parameter Error Response
	dword value;
};

/**
 * NVMe-MI Message (implicitly include MIC)
 */
union nvme_mi_msg {
	struct {
		union nvme_mi_msg_header nmh;
		uint8_t msg_data[NVME_MI_MSG_SIZE - sizeof(union nvme_mi_msg_header)];
	};
	uint8_t raw_data[NVME_MI_MSG_SIZE];
};

/**
 * NVMe-MI Command Request Message Format (implicitly include MIC)
 */
union nvme_mi_req_msg {
	struct {
		// TBD: Use union nvme_mi_req_dw req_dw
		union nvme_mi_msg_header nmh;                   // NVMe-MI Message Header (NMH)
		uint8_t opc;                                       // Opcode (OPC)
		uint8_t rsvd[3];
		union nvme_mi_nmd0 nmd0;                        // NVMe Management Dword 0 (NMD0)
		union nvme_mi_nmd1 nmd1;                        // NVMe Management Dword 1 (NMD1)
		uint8_t req_data[NVME_MI_MSG_SIZE - sizeof(union nvme_mi_req_dw)];
	};
	uint8_t raw_data[NVME_MI_MSG_SIZE];
};

/**
 * NVMe-MI Command Response Message Format (implicitly include MIC)
 */
union nvme_mi_res_msg {
	struct {
		union nvme_mi_msg_header nmh;   // NVMe-MI Message Header (NMH)
		union nvme_mi_resp nmresp;      // NVMe Management Response
		uint8_t res_data[NVME_MI_MSG_SIZE - sizeof(union nvme_mi_msg_header) - sizeof(union nvme_mi_resp)];
	};
	uint8_t raw_data[NVME_MI_MSG_SIZE];
};

// Get Log Page – Command Dword 10
union get_log_page_cdw10 {
	struct {
		dword lid   : 8;        // Log Page Identifier (LID)
		dword lsp   : 4;        // Log Specific Field (LSP)
		dword rsvd  : 3;        // Reserved
		dword rae   : 1;        // Retain Asynchronous Event (RAE)
		dword numdl : 16;       // Number of Dwords Lower (NUMDL)
	};
	dword value;
};

// Get Log Page – Command Dword 11
union get_log_page_cdw11 {
	struct  {
		dword numdu : 16;       // Number of Dwords Upper (NUMDU)
		dword lsi   : 16;       // Log Specific Identifier (LSI)
	};
	dword value;
};

// Get Log Page – Command Dword 12
union get_log_page_cdw12 {
	struct {
		dword lpol  : 32;       // Log Page Offset Lower (LPOL)
	};
	dword value;
};

// Get Log Page – Command Dword 13
union get_log_page_cdw13 {
	struct {
		dword lpou  : 32;       // Log Page Offset Upper (LPOU)
	};
	dword value;
};

// Get Log Page – Command Dword 14
union get_log_page_cdw14 {
	struct {
		dword uuid  : 7;
		dword rsvd  : 25;       // Reserved
	};
	dword value;
};

union nvme_mi_adm_get_log_page_dw {
	struct {
		union get_log_page_cdw10 cdw10; // Get Log Page – Command Dword 10
		union get_log_page_cdw11 cdw11; // Get Log Page – Command Dword 11
		union get_log_page_cdw12 cdw12; // Get Log Page – Command Dword 12
		union get_log_page_cdw13 cdw13; // Get Log Page – Command Dword 13
		union get_log_page_cdw14 cdw14; // Get Log Page – Command Dword 14
	};
	uint8_t raw_data[20];
};

// Identify – Command Dword 10
union identify_cdw10 {
	struct {
		uint32_t cns   : 8;     // Controller or Namespace Structure (CNS)
		uint32_t rsvd  : 8;
		uint32_t cntid : 16;    // Controller Identifier (CNTID)
	};
	uint32_t value;
};

// Identify – Command Dword 11
union identify_cdw11 {
	struct {
		uint32_t nvmsetid : 16; // NVM Set Identifier (NVMSETID)
		uint32_t rsvd     : 16;
	};
	uint32_t value;
};

// Identify – Command Dword 14
union identify_cdw14 {
	struct {
		uint32_t uuid_index : 7;        // UUID Index
		uint32_t rsvd       : 25;
	};
	uint32_t value;
};

union nvme_mi_adm_identify {
	struct {
		union identify_cdw10 cdw10;     // Identify – Command Dword 10
		union identify_cdw11 cdw11;     // Identify – Command Dword 11
		uint32_t rvsd_cdw12;
		uint32_t rvsd_cdw13;
		union identify_cdw14 cdw14;     // Identify – Command Dword 14
	};
	uint8_t raw_data[20];
};

union get_feat_cdw10 {
	struct {
		uint32_t fid  : 8;
		uint32_t sel  : 3;
		uint32_t rsvd : 21;
	};
	uint32_t value;
};

union get_feat_cdw14 {
	struct {
		uint32_t uuid_index : 7;
		uint32_t rsvd       : 25;
	};
	uint32_t value;
};

union nvme_mi_adm_get_features {
	struct {

		union get_feat_cdw10 cdw10;
		uint32_t rsvd_cdw11;
		uint32_t rvsd_cdw12;
		uint32_t rvsd_cdw13;
		union get_feat_cdw14 cdw14;
	};
	uint8_t raw_data[20];
};

/**
 * NVMe Admin Command Request Format (without MCTP Message Header, Request Data,
 * and MIC)
 */
struct nvme_mi_adm_req_dw {
	// cdw0
	union {
		struct {
			uint8_t opc;
			uint8_t cflgs;
			word ctrid;
		};
		dword sqedw0;
	};
	// cdw1 ~ cdw5
	union {
		dword nsid;
		dword sqedw1;
	};
	dword sqedw2;
	dword sqedw3;
	dword sqedw4;
	dword sqedw5;
	// cdw6 ~ cdw7
	union {
		struct {
			dword dofst;
			dword dlen;
		};
		qword prp1;
	};
	// cdw8 ~ cdw9
	union {
		dword rsvd[2];
		qword prp2_list;
	};
	// cdw10 ~ cdw15
	union {
		union nvme_mi_req_dw mi_cmd;
		union nvme_mi_adm_get_log_page_dw get_log_page;
		union nvme_mi_adm_identify identify;
		union nvme_mi_adm_get_features get_feat;
		struct {
			dword sqedw10;
			dword sqedw11;
			dword sqedw12;
			dword sqedw13;
			dword sqedw14;
			dword sqedw15;
		};
	};
};

/**
 * NVMe Admin Command Response Format (without MCTP Message Header (NVMe-MI
 * Message Header), Status, Request Data, and MIC)
 *
 * 6 NVM Express Admin Command Set, Figure 119
 */
struct nvme_mi_adm_res_dw {
	dword cqedw0;
	dword cqedw1;
	union {
		struct {
			dword cmd_id : 16;
			dword p      : 1;
			dword sf     : 15;
		};
		dword cqedw3;
	};
};

#pragma pack(pop)

int nvme_mi_send_admin_command(struct aa_args *args, uint8_t opc, union nvme_mi_msg *msg,
                               size_t req_size);
int nvme_mi_message_handle(const union nvme_mi_msg *msg, uint16_t size);

#endif // NVME_MI_H