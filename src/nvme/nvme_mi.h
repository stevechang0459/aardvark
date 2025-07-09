#ifndef NVME_MI_H
#define NVME_MI_H

#include "types.h"
#include "libnvme_mi_mi.h"
#include "libnvme_mi_mi.h"

#include <stdbool.h>
#include <stdlib.h>

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

// // NVMe-MI Message Type (NMIMT)
// enum nvme_mi_msg_type {
//      NMIMT_CTRLP = 0,                    // Control Primitive
//      NMIMT_MI = 1,                       // NVMe-MI Command
//      NMIMT_ADMIN,                        // NVMe Admin Command
//      NMIMT_RSVD,                         // Reserved
//      NMIMT_PCIE,                         // PCIe Command
//      // 5h to Fh: Reserved
//      NMIMT_MAX,
// };

enum nvme_mi_port_id {
	NVME_MI_PORT_ID_PCIE = 0,
	NVME_MI_PORT_ID_SMBUS = 1,
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
		uint32_t mt    : 7;     // Bit[6:0]   Message Type (MT)
		uint32_t ic    : 1;     // Bit[7]     Integrity Check (IC)
		uint32_t csi   : 1;     // Bit[8]     Command Slot Identifier (CSI)
		uint32_t rsvd1 : 2;     // Bit[10:9]  Reserved
		uint32_t nmimt : 4;     // Bit[14:11] NVMe-MI Message Type (NMIMT)
		uint32_t ror   : 1;     // Bit[15:15] Request or Response (ROR)
		uint32_t meb   : 1;     // Bit[16:16] Management Endpoint Buffer (MEB)
		uint32_t ciap  : 1;     // Bit[17:17] Command Initiated Auto Pause (CIAP)
		uint32_t rsvd2 : 14;    // Bit[31:18] Reserved
	};
	uint32_t value;
};

// Read NVMe-MI Data Structure – NVMe Management Dword 0
struct nmd0_rnmds {
	uint32_t ctrlid : 16;           // Bit[15:0] Controller Identifier (CTRLID)
	uint32_t portid : 8;            // Bit[23:16] Port Identifier (PORTID)
	uint32_t dtyp   : 8;            // Bit[31:24] Data Structure Type (DTYP)
};

// Controller Health Status Poll – NVMe Management Dword 0
struct nmd0_chsp {
	uint32_t sctlid  : 16;          // Bit[15:0] Starting Controller ID (SCTLID)
	uint32_t maxrent : 8;           // Bit[23:16] Maximum Response Entries (MAXRENT)
	uint32_t incf    : 1;           // Bit[24:24] Include PCI Functions (INCF)
	uint32_t incpf   : 1;           // Bit[25:25] Include SR-IOV Physical Functions (INCPF)
	uint32_t incvf   : 1;           // Bit[26:26] Include SR-IOV Virtual Functions (INCVF)
	uint32_t rsvd    : 4;           // Bit[30:27] Reserved
	uint32_t all     : 1;           // Bit[31:31] Report All (ALL)
};

// Configuration Set/Get, SMBus/I2C Frequency – NVMe Management Dword 0
struct nmd0_config_sif {
	uint32_t cfg_id  : 8;           // Bit[7:0] Configuration Identifier
	uint32_t sif     : 4;           // Bit[11:8] SMBus/I2C Frequency
	uint32_t rsvd    : 12;          // Bit[23:12] Reserved
	uint32_t port_id : 8;           // Bit[31:24] Port Identifier
};

// Configuration Set, Health Status Change - NVMe Management Dword 0
struct nmd0_config_hsc {
	uint32_t cfg_id  : 8;           // Bit[7:0] Configuration Identifier
	uint32_t rsvd    : 24;          // Bit[31:8] Reserved
};

// Configuration Set/Get, MCTP Transmission Unit Size – NVMe Management Dword 0
struct nmd0_config_mtus {
	uint32_t cfg_id  : 8;           // Bit[7:0] Configuration Identifier
	uint32_t rsvd    : 16;          // Bit[23:8] Reserved
	uint32_t port_id : 8;           // Bit[31:24] Port Identifier
};

// Configuration Set/Get – NVMe Management Dword 0
union nmd0_config {
	struct {
		uint32_t cfg_id   : 8;  // Bit[7:0] Configuration Identifier
		uint32_t rsvd     : 16; // Bit[23:8] Reserved
		uint32_t port_id  : 8;  // Bit[31:24] Port Identifier
	};
	struct nmd0_config_sif sif;     // SMBus/I2C Frequency
	struct nmd0_config_hsc hsc;     // Health Status Change
	struct nmd0_config_mtus mtus;   // MCTP Transmission Unit Size
	uint32_t value;
};

// VPD Read - NVMe Management Dword 0
struct nmd0_vpdr {
	uint32_t dofst : 16;            // Bit[15:0] Data Offset (DOFST)
	uint32_t rsvd  : 16;            // Bit[31:16] Data Length (DLEN)
};

// Reset - NVMe Management Dword 0
struct nmd0_rst {
	uint32_t rsvd       : 24;       // Bit[23:0] Reserved
	uint32_t reset_type : 8;        // Bit[31:24] Reset Type
};

// Shutdown - NVMe Management Dword 0
struct nmd0_sh {
	uint32_t rsvd    : 24;          // Bit[23:0] Reserved
	uint32_t sh_type : 8;           // Bit[31:24] Shutdown Type
};

/**
 * NVMe Management Dword 0 (NMD0) - Command Dword 12 (CDW12)
 */
union nvme_mi_nmd0 {
	struct nmd0_rnmds rnmds;        // Read NVMe-MI Data Structure – NVMe Management Dword 0
	struct nmd0_chsp chsp;          // Controller Health Status Poll – NVMe Management Dword 0
	union nmd0_config cfg;          // Configuration Set/Get – NVMe Management Dword 0
	struct nmd0_vpdr vpdr;          // VPD Read – NVMe Management Dword 0
	struct nmd0_rst rst;            // Reset – NVMe Management Dword 0
	struct nmd0_sh sh;              // Shutdown – NVMe Management Dword 0
	uint32_t value;
};

// Read NVMe-MI Data Structure - NVMe Management Dword 1
struct nmd1_rnmds {
	uint32_t iocsi : 8;             // Bit[7:0] I/O Command Set Identifier (IOCSI)
	uint32_t rsvd  : 24;            // Bit[31:8] Reserved
};

// NVM Subsystem Health Status Poll - NVMe Management Dword 1
struct nmd1_nshsp {
	uint32_t rsvd : 31;
	uint32_t cs   : 1;              // Clear Status (CS)
};

// Controller Health Status Poll – NVMe Management Dword 1
struct nmd1_chsp {
	uint32_t csts  : 1;             // Bit[0] Controller Status Changes (CSTS)
	uint32_t ctemp : 1;             // Bit[1] Composite Temperature Changes (CTEMP)
	uint32_t pdlu  : 1;             // Bit[2] Percentage Used (PDLU)
	uint32_t spare : 1;             // Bit[3] Available Spare (SPARE)
	uint32_t cwarn : 1;             // Bit[4] Critical Warning (CWARN)
	uint32_t rsvd  : 26;            // Bit[30:5] Reserved
	uint32_t ccf   : 1;             // Bit[31:31] Clear Changed Flags (CCF)
};

struct nmd1_config_sif {
	uint32_t rsvd;
};

// Configuration Set - Health Status Change
union nmd1_config_hsc {
	struct {
		uint32_t rdy    : 1;    // Bit[0] Ready (RDY)
		uint32_t cfs    : 1;    // Bit[1] Controller Fatal Status (CFS)
		uint32_t shst   : 1;    // Bit[2] Shutdown Status (SHST)
		uint32_t nssro  : 1;    // Bit[3] NVM Subsystem Reset Occurred (NSSRO)
		uint32_t ceco   : 1;    // Bit[4] Controller Enable Change Occurred (CECO)
		uint32_t nac    : 1;    // Bit[5] Namespace Attribute Changed (NAC)
		uint32_t fa     : 1;    // Bit[6] Firmware Activated (FA)
		uint32_t cschng : 1;    // Bit[7] Controller Status Change (CSCHNG)
		uint32_t ctemp  : 1;    // Bit[8] Composite Temperature (CTEMP)
		uint32_t pdlu   : 1;    // Bit[9] Percentage Used (PDLU)
		uint32_t spare  : 1;    // Bit[10] Available Spare (SPARE)
		uint32_t cwarn  : 1;    // Bit[11] Critical Warning (CWARN)
		uint32_t rsvd   : 20;   // Bit[31:12] Reserved
	};
	uint32_t value;
};

// MCTP Transmission Unit Size – NVMe Management Dword 1
union nmd1_config_mtus {
	struct {
		uint32_t mtus : 16;     // Bit[15:0] MCTP Transmission Unit Size
		uint32_t rsvd : 16;     // Bit[31:16] Reserved
	};
	uint32_t value;
};

// Configuration Set/Get – NVMe Management Dword 0
union nmd1_config {
	struct nmd1_config_sif sif;     // SMBus/I2C Frequency
	union nmd1_config_hsc hsc;      // Health Status Change
	union nmd1_config_mtus mtus;    // MCTP Transmission Unit Size
	uint32_t value;
};

// VPD Read - NVMe Management Dword 1
struct nmd1_vpdr {
	uint32_t dlen : 16;             // Bit[15:0] Data Length (DLEN)
	uint32_t rsvd : 16;             // Bit[31:16] Data Offset (DOFST)
};

/**
 * NVMe Management Dword 1 (NMD1) - Command Dword 13 (CDW13)
 */
union nvme_mi_nmd1 {
	struct nmd1_rnmds rnmds;        // Read NVMe-MI Data Structure
	struct nmd1_nshsp nshsp;        // NVM Subsystem Health Status Poll
	struct nmd1_chsp chsp;          // Controller Health Status Poll
	union nmd1_config cfg;          // Configuration Set/Get
	struct nmd1_vpdr vpdr;          // VPD Read
	uint32_t value;
};

// Read NVMe-MI Data Structure (00h) - NVMe Management Response
struct nvme_mi_resp_rnmds {
	uint32_t status        : 8;
	uint32_t resp_data_len : 16;    // Bit[15:0] Response Data Length
	uint32_t rsvd          : 8;     // Bit[23:16] Reserved
};

// Controller Health Status Poll (02h) - NVMe Management Response
struct nvme_mi_resp_chsp {
	uint32_t status : 8;            // bit[7:0] Response Message Status
	uint32_t rsvd   : 16;           // bit[23:8] Reserved
	uint32_t rent   : 8;            // bit[31:24] Response Entries (RENT)
};

// Generic Error Response
struct nvme_mi_resp_error {
	uint32_t status : 8;            // bit[7:0] Response Message Status
	uint32_t rsvd   : 24;           // bit[31:8] Reserved
};

// Invalid Parameter Error Response
struct nvme_mi_resp_invld_para {

	uint32_t status : 8;            // Bit[7:0] Response Message Status
	/**
	 * Bit[10:8]
	 * Least-significant bit in the least-significant uint8_t of the Request
	 * Message of the parameter that contained the error. Valid values are 0 to
	 * 7.
	 */
	uint32_t lsbit : 3;
	// Bit[15:11] Reserved
	uint32_t rsvd : 5;
	/**
	 * Bit[31:16]
	 * Least-significant uint8_t of the Request Message of the parameter that
	 * contained the error. If the error is beyond uint8_t 65,535, then the value
	 * 65,535 is reported in this field.
	 */
	uint32_t lsbyte : 16;
};

/**
 * NVMe-MI Command Request Message Format (without Request Data and MIC)
 */
union nvme_mi_req_dw {
	struct {
		union nvme_mi_msg_header nmh;   // Command Dword 10 (CDW10) - NVMe-MI Message Header (NMH)
		uint8_t opc;                    // Command Dword 11 (CDW11) - Opcode (OPC)
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
		uint32_t status : 8;                    // bit[7:0] Tunneled Status (STATUS)
		uint32_t nmresp : 24;                   // bit[31:8] NVMe Management Response (NMRESP)
	};
	struct nvme_mi_resp_rnmds rnmds;                // NVMe Management Response - Read NVMe-MI Data Structure
	struct nvme_mi_resp_chsp chsp;                  // NVMe Management Response - Controller Health Status Poll
	struct nvme_mi_resp_error error;                // NVMe Management Response - Generic Error Response
	struct nvme_mi_resp_invld_para invld_para;      // NVMe Management Response - Invalid Parameter Error Response
	uint32_t value;
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
		union nvme_mi_msg_header nmh;   // NVMe-MI Message Header (NMH)
		uint8_t opc;                    // Opcode (OPC)
		uint8_t rsvd[3];
		union nvme_mi_nmd0 nmd0;        // NVMe Management Dword 0 (NMD0)
		union nvme_mi_nmd1 nmd1;        // NVMe Management Dword 1 (NMD1)
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
		uint32_t lid   : 8;     // Log Page Identifier (LID)
		uint32_t lsp   : 4;     // Log Specific Field (LSP)
		uint32_t rsvd  : 3;     // Reserved
		uint32_t rae   : 1;     // Retain Asynchronous Event (RAE)
		uint32_t numdl : 16;    // Number of Dwords Lower (NUMDL)
	};
	uint32_t value;
};

// Get Log Page – Command Dword 11
union get_log_page_cdw11 {
	struct  {
		uint32_t numdu : 16;    // Number of Dwords Upper (NUMDU)
		uint32_t lsi   : 16;    // Log Specific Identifier (LSI)
	};
	uint32_t value;
};

// Get Log Page – Command Dword 12
union get_log_page_cdw12 {
	struct {
		uint32_t lpol  : 32;    // Log Page Offset Lower (LPOL)
	};
	uint32_t value;
};

// Get Log Page – Command Dword 13
union get_log_page_cdw13 {
	struct {
		uint32_t lpou  : 32;    // Log Page Offset Upper (LPOU)
	};
	uint32_t value;
};

// Get Log Page – Command Dword 14
union get_log_page_cdw14 {
	struct {
		uint32_t uuid  : 7;
		uint32_t rsvd  : 25;    // Reserved
	};
	uint32_t value;
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
			uint16_t ctrid;
		};
		uint32_t sqedw0;
	};
	// cdw1 ~ cdw5
	union {
		uint32_t nsid;
		uint32_t sqedw1;
	};
	uint32_t sqedw2;
	uint32_t sqedw3;
	uint32_t sqedw4;
	uint32_t sqedw5;
	// cdw6 ~ cdw7
	union {
		struct {
			uint32_t dofst;
			uint32_t dlen;
		};
		uint64_t prp1;
	};
	// cdw8 ~ cdw9
	union {
		uint32_t rsvd[2];
		uint64_t prp2_list;
	};
	// cdw10 ~ cdw15
	union {
		union nvme_mi_req_dw mi_cmd;
		union nvme_mi_adm_get_log_page_dw get_log_page;
		union nvme_mi_adm_identify identify;
		union nvme_mi_adm_get_features get_feat;
		struct {
			uint32_t sqedw10;
			uint32_t sqedw11;
			uint32_t sqedw12;
			uint32_t sqedw13;
			uint32_t sqedw14;
			uint32_t sqedw15;
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
	uint32_t cqedw0;
	uint32_t cqedw1;
	union {
		struct {
			uint32_t cmd_id : 16;
			uint32_t p      : 1;
			uint32_t sf     : 15;
		};
		uint32_t cqedw3;
	};
};

/**
 * NVM Subsystem Status (NSS), NVM Subsystem Health Data Structure
 *
 * This field indicates the status of the NVM Subsystem.
 */
union nvm_subsys_sts {
	struct {
		uint8_t rsvd1 : 2;      // Bit[1:0] Reserved
		uint8_t p1la  : 1;      // Bit[2:2] Port 1 PCIe Link Active (P1LA)
		uint8_t p0la  : 1;      // Bit[3:3] Port 0 PCIe Link Active (P0LA)
		uint8_t rnr   : 1;      // Bit[4:4] Reset Not Required (RNR)
		uint8_t df    : 1;      // Bit[5:5] Drive Functional (DF)
		uint8_t rsvd2 : 2;      // Bit[7:6] Reserved
	};
	uint8_t value;
};

/**
 * Composite Controller Status (CCS), NVM Subsystem Health Data Structure
 *
 * This field reports the composite status of all Controllers in the NVM
 * Subsystem.
 */
union comp_ctrl_sts {
	struct {
		uint16_t rdy   : 1;     // Bit[0] Ready (RDY)
		uint16_t cfs   : 1;     // Bit[1] Controller Fatal Status (CFS)
		uint16_t shst  : 1;     // Bit[2] Shutdown Status (SHST)
		uint16_t rsvd1 : 1;     // Bit[3] Reserved
		uint16_t nssro : 1;     // Bit[4] NVM Subsystem Reset Occurred (NSSRO)
		uint16_t ceco  : 1;     // Bit[5] Controller Enable Change Occurred (CECO)
		uint16_t nac   : 1;     // Bit[6] Namespace Attribute Changed (NAC)
		uint16_t fa    : 1;     // Bit[7] Firmware Activated (FA)
		uint16_t csts  : 1;     // Bit[8] Controller Status Change (CSTS)
		uint16_t ctemp : 1;     // Bit[9] Composite Temperature Change (CTEMP)
		uint16_t pdlu  : 1;     // Bit[10] Percentage Used (PDLU)
		uint16_t spare : 1;     // Bit[11] Available Spare (SPARE)
		uint16_t cwarn : 1;     // Bit[12] Critical Warning (CWARN)
		uint16_t rsvd2 : 3;     // Bit[15:13] Reserved
	};
	uint16_t value;
};

// NVM Subsystem Health Data Structure (NSHDS)
struct nvm_subsys_health_ds {
	union nvm_subsys_sts nss;       // Byte[0] NVM Subsystem Status (NSS)
	uint8_t sw;                     // Byte[1] Smart Warnings (SW)
	uint8_t ctemp;                  // Byte[2] Composite Temperature (CTEMP)
	uint8_t pdlu;                   // Byte[3] Percentage Drive Life Used (PDLU)
	union comp_ctrl_sts ccs;        // Byte[5:4] Composite Controller Status (CCS)
	uint16_t rsvd;                  // Byte[7:6] Reserved
};

// Controller Status (CSTS), Controller Health Data Structure
union chds_csts {
	struct {
		uint16_t rdy   : 1;     // Bit[0] Ready (RDY)
		uint16_t cfs   : 1;     // Bit[1] Controller Fatal Status (CFS)
		uint16_t shst  : 2;     // Bit[3:2] Shutdown Status (SHST)
		uint16_t nssro : 1;     // Bit[4] NVM Subsystem Reset Occurred (NSSRO)
		uint16_t ceco  : 1;     // Bit[5] Controller Enable Change Occurred (CECO)
		uint16_t nac   : 1;     // Bit[6] Namespace Attribute Changed (NAC)
		uint16_t fa    : 1;     // Bit[7] Firmware Activated (FA)
		uint16_t rsvd  : 8;     // Bit[15:8] Reserved
	};
	uint16_t value;
};

// Critical Warning (CWARN), Controller Health Data Structure
union chds_cwarn {
	struct {
		uint8_t st   : 1;       // Bit[0] Spare Threshold (ST)
		uint8_t taut : 1;       // Bit[1] Temperature Above or Under Threshold (TAUT)
		uint8_t rd   : 1;       // Bit[2] Reliability Degraded (RD)
		uint8_t ro   : 1;       // Bit[3] Read Only (RO)
		uint8_t vmbf : 1;       // Bit[4] Volatile Memory Backup Failed (VMBF)
		uint8_t pmre : 1;       // Bit[5] Persistent Memory Region Error (PMRE)
		uint8_t rsvd : 2;       // Bit[7:6] Reserved
	};
	uint8_t value;
};

// Controller Health Data Structure (CHDS)
struct controller_health_ds {
	uint16_t ctlid;                 // Byte[1:0] Controller Identifier (CTLID)
	union chds_csts csts;           // Byte[3:2] Controller Status (CSTS)
	uint16_t ctemp;                 // Byte[4:5] Composite Temperature (CTEMP)
	uint8_t pdlu;                   // Byte[6] Percentage Used (PDLU)
	uint8_t spare;                  // Byte[7] Available Spare (SPARE)
	union chds_cwarn cwarn;         // Byte[8] Critical Warning (CWARN)
	uint8_t rsvd[7];                // Byte[15:9] Reserved
};

// Controller Health Status Changed Flags (CHSCF)
union chscf {
	struct {
		uint16_t rdy   : 1;     // Bit[0] Ready (RDY)
		uint16_t cfs   : 1;     // Bit[1] Controller Fatal Status (CFS)
		uint16_t shst  : 1;     // Bit[2] Shutdown Status (SHST)
		uint16_t rsvd1 : 1;     // Bit[3] Reserved
		uint16_t nssro : 1;     // Bit[4] NVM Subsystem Reset Occurred (NSSRO)
		uint16_t ceco  : 1;     // Bit[5] Controller Enable Change Occurred (CECO)
		uint16_t nac   : 1;     // Bit[6] Namespace Attribute Changed (NAC)
		uint16_t fa    : 1;     // Bit[7] Firmware Activated (FA)
		//
		uint16_t csts  : 1;     // Bit[8] Controller Status Change (CSTS)
		uint16_t ctemp : 1;     // Bit[9] Composite Temperature Change (CTEMP)
		uint16_t pdlu  : 1;     // Bit[10] Percentage Used (PDLU)
		uint16_t spare : 1;     // Bit[11] Available Spare (SPARE)
		uint16_t cwarn : 1;     // Bit[12] Critical Warning (CWARN)
		uint16_t rsvd2 : 3;     // Bit[15:13] Reserved
	};
	uint16_t value;
};

#pragma pack(pop)

int nvme_mi_send_admin_command(struct aa_args *args, uint8_t opc, union nvme_mi_msg *msg,
                               size_t req_size);
int nvme_mi_message_handle(const union nvme_mi_msg *msg, uint16_t size);
int nvme_mi_mi_subsystem_health_status_poll(struct aa_args *args, bool cs);
int nvme_mi_mi_controller_health_status_poll(struct aa_args *args, bool ccf);
int nvme_mi_mi_config_get(struct aa_args *args, union nvme_mi_nmd0 nmd0, union nvme_mi_nmd1 nmd1);
int nvme_mi_mi_config_get_sif(struct aa_args *args);
int nvme_mi_mi_config_get_hsc(struct aa_args *args);
int nvme_mi_mi_config_get_mtus(struct aa_args *args);
int nvme_mi_mi_config_set(struct aa_args *args, union nvme_mi_nmd0 nmd0, union nvme_mi_nmd1 nmd1);
int nvme_mi_mi_config_set_sif(struct aa_args *args, uint8_t port_id, uint8_t freq_sel);
int nvme_mi_mi_config_set_hsc(struct aa_args *args, union nmd1_config_hsc hsc);
int nvme_mi_mi_config_set_mtus(struct aa_args *args, uint8_t port_id, union nmd1_config_mtus mtus);
int nvme_mi_mi_data_read_nvm_subsys_info(struct aa_args *args);

#endif // NVME_MI_H