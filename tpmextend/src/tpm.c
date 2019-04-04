/*
 * tpm.c
 *
 *  Created on: 26-June-2018
 *      Author: Arvind Rawat
 */

#include "common.h"
#include "tpm.h"

/*
 * specified as minimum cmd buffer size should be supported by all 1.2 TPM
 * device in the TCG_PCClientTPMSpecification_1-20_1-00_FINAL.pdf
 */
#define TPM_CMD_SIZE_MAX	768
#define TPM_RSP_SIZE_MAX	768

#define TPM_TAG_RQU_COMMAND     0x00C1
#define TPM_ORD_PCR_EXTEND      0x00000014
#define TPM_ORD_PCR_READ        0x00000015
#define TPM_ORD_PCR_RESET       0x000000C8
#define TPM_ORD_NV_READ_VALUE   0x000000CF
#define TPM_ORD_NV_WRITE_VALUE  0x000000CD
#define TPM_ORD_GET_CAPABILITY  0x00000065
#define TPM_ORD_SEAL            0x00000017
#define TPM_ORD_UNSEAL          0x00000018
#define TPM_ORD_OSAP            0x0000000B
#define TPM_ORD_OIAP            0x0000000A
#define TPM_ORD_SAVE_STATE      0x00000098
#define TPM_ORD_GET_RANDOM      0x00000046

#define TPM_TAG_PCR_INFO_LONG   0x0006
#define TPM_TAG_STORED_DATA12   0x0016

static uint8_t     		cmd_buf[TPM_CMD_SIZE_MAX];
static uint8_t     		rsp_buf[TPM_RSP_SIZE_MAX];
#define WRAPPER_IN_BUF          (cmd_buf + CMD_HEAD_SIZE)
#define WRAPPER_OUT_BUF         (rsp_buf + RSP_HEAD_SIZE)
#define WRAPPER_IN_MAX_SIZE     (TPM_CMD_SIZE_MAX - CMD_HEAD_SIZE)
#define WRAPPER_OUT_MAX_SIZE    (TPM_RSP_SIZE_MAX - RSP_HEAD_SIZE)

static uint32_t tpm_write_cmd_fifo(uint8_t *in, uint32_t in_size, uint8_t *out, uint32_t *out_size) {

    //uint32_t            i, rsp_size, offset, ret;
    //uint16_t            row_size;
    //tpm_reg_access_t    reg_acc;
    //tpm_reg_sts_t       reg_sts;

    if ( in == NULL || out == NULL || out_size == NULL ) {
        fprintf(STDOUT, "TPM: Invalid parameter for tpm_write_cmd_fifo()\n");
        return TPM_BAD_PARAMETER;
    }
    if ( in_size < CMD_HEAD_SIZE || *out_size < RSP_HEAD_SIZE ) {
        fprintf(STDOUT, "TPM: in/out buf size must be larger than 10 bytes\n");
        return TPM_BAD_PARAMETER;
    }

#ifdef TPM_TRACE
    {
        fprintf(STDOUT, "TPM: cmd size = %d\nTPM: cmd content: ", in_size);
        print_hex("TPM: \t", in, in_size);
    }
#endif
	int ret = write(tpmfd, in, in_size);
	if(ret<0){
        fprintf(STDOUT, "write failed, ret %d\n", ret);
        close(tpmfd);
        return ret;
    }
/*
    /* write the command to the TPM FIFO *
    offset = 0;
    do {
        i = 0;
        do {
            read_tpm_reg(locality, TPM_REG_STS, &reg_sts);
            /* find out how many bytes the TPM can accept in a row *
            row_size = reg_sts.burst_count;
            if ( row_size > 0 )
                break;
            else
                cpu_relax();
            i++;
        } while ( i <= TPM_CMD_WRITE_TIME_OUT );
        if ( i > TPM_CMD_WRITE_TIME_OUT ) {
            fprintf(KERN_ERR"TPM: write cmd timeout\n");
            ret = TPM_FAIL;
            goto RelinquishControl;
        }

        for ( ; row_size > 0 && offset < in_size; row_size--, offset++ )
            write_tpm_reg(locality, TPM_REG_DATA_FIFO,
                          (tpm_reg_data_fifo_t *)&in[offset]);
    } while ( offset < in_size );

    i = 0;
    do {
        read_tpm_reg(locality,TPM_REG_STS, &reg_sts);
#ifdef TPM_TRACE
        fprintf(STDOUT,"Wait on Expect = 0, Status register %02x\n", reg_sts._raw[0]);
#endif
        if ( reg_sts.sts_valid == 1 && reg_sts.expect == 0 )
            break;
        else
            cpu_relax();
        i++;
    } while ( i <= TPM_DATA_AVAIL_TIME_OUT );
    if ( i > TPM_DATA_AVAIL_TIME_OUT ) {
        fprintf(KERN_ERR"TPM: wait for expect becoming 0 timeout\n");
        ret = TPM_FAIL;
        goto RelinquishControl;
    }

    /* command has been written to the TPM, it is time to execute it. *
    memset(&reg_sts, 0,  sizeof(reg_sts));
    reg_sts.tpm_go = 1;
    write_tpm_reg(locality, TPM_REG_STS, &reg_sts);

    /* check for data available *
    i = 0;
    do {
        read_tpm_reg(locality,TPM_REG_STS, &reg_sts);
#ifdef TPM_TRACE
        fprintf(STDOUT,"Waiting for DA Flag, Status register %02x\n", reg_sts._raw[0]);
#endif
        if ( reg_sts.sts_valid == 1 && reg_sts.data_avail == 1 )
            break;
        else
            cpu_relax();
        i++;
    } while ( i <= TPM_DATA_AVAIL_TIME_OUT );
    if ( i > TPM_DATA_AVAIL_TIME_OUT ) {
        fprintf(KERN_ERR"TPM: wait for data available timeout\n");
        ret = TPM_FAIL;
        goto RelinquishControl;
    }

    rsp_size = 0;
    offset = 0;
    do {
        /* find out how many bytes the TPM returned in a row *
        i = 0;
        do {
            read_tpm_reg(locality, TPM_REG_STS, &reg_sts);
            row_size = reg_sts.burst_count;
            if ( row_size > 0 )
                break;
            else
                cpu_relax();
            i++;
        } while ( i <= TPM_RSP_READ_TIME_OUT );
        if ( i > TPM_RSP_READ_TIME_OUT ) {
            fprintf(KERN_ERR"TPM: read rsp timeout\n");
            ret = TPM_FAIL;
            goto RelinquishControl;
        }

        for ( ; row_size > 0 && offset < *out_size; row_size--, offset++ ) {
            if ( offset < *out_size )
                read_tpm_reg(locality, TPM_REG_DATA_FIFO,
                             (tpm_reg_data_fifo_t *)&out[offset]);
            else {
                /* discard the responded bytes exceeding out buf size *
                tpm_reg_data_fifo_t discard;
                read_tpm_reg(locality, TPM_REG_DATA_FIFO,
                             (tpm_reg_data_fifo_t *)&discard);
            }

            /* get outgoing data size *
            if ( offset == RSP_RST_OFFSET - 1 ) {
                reverse_copy(&rsp_size, &out[RSP_SIZE_OFFSET],
                             sizeof(rsp_size));
            }
        }
    } while ( offset < RSP_RST_OFFSET ||
              (offset < rsp_size && offset < *out_size) );

    *out_size = (*out_size > rsp_size) ? rsp_size : *out_size;

    /* out buffer contains the complete outgoing data, get return code *
    reverse_copy(&ret, &out[RSP_RST_OFFSET], sizeof(ret));

#ifdef TPM_TRACE
    {
        fprintf(STDOUT, "TPM: response size = %d\n", *out_size);
        fprintf(STDOUT, "TPM: response content: ");
        print_hex("TPM: \t", out, *out_size);
    }
#endif

    memset(&reg_sts, 0, sizeof(reg_sts));
    reg_sts.command_ready = 1;
    write_tpm_reg(locality, TPM_REG_STS, &reg_sts);

RelinquishControl:
    /* deactivate current locality *
    reg_acc._raw[0] = 0;
    reg_acc.active_locality = 1;
    write_tpm_reg(locality, TPM_REG_ACCESS, &reg_acc);
*/
    return ret;
}

static uint32_t _tpm_submit_cmd(uint16_t tag, uint32_t cmd, uint32_t arg_size, uint32_t *out_size)
{
    uint32_t    ret;
    uint32_t    cmd_size, rsp_size = 0;

    if ( out_size == NULL ) {
        fprintf(STDOUT, "TPM: invalid param for _tpm_submit_cmd()\n");
        return TPM_BAD_PARAMETER;
    }

    /*
     * real cmd size should add 10 more bytes:
     *      2 bytes for tag
     *      4 bytes for size
     *      4 bytes for ordinal
     */
    cmd_size = CMD_HEAD_SIZE + arg_size;

    if ( cmd_size > TPM_CMD_SIZE_MAX ) {
        fprintf(STDOUT, "TPM: cmd exceeds the max supported size.\n");
        return TPM_BAD_PARAMETER;
    }

    /* copy tag, size & ordinal into buf in a reversed byte order */
    reverse_copy(cmd_buf, &tag, sizeof(tag));
    reverse_copy(cmd_buf + CMD_SIZE_OFFSET, &cmd_size, sizeof(cmd_size));
    reverse_copy(cmd_buf + CMD_ORD_OFFSET, &cmd, sizeof(cmd));


	fprintf(STDOUT, "cmd_buf ready, call tpm_write_cmd_fifo next\n");
	
    rsp_size = RSP_HEAD_SIZE + *out_size;
    rsp_size = (rsp_size > TPM_RSP_SIZE_MAX) ? TPM_RSP_SIZE_MAX: rsp_size;
    ret = tpm_write_cmd_fifo(cmd_buf, cmd_size, rsp_buf, &rsp_size);

	fprintf(STDOUT, "after tpm_write_cmd_fifo, ret=%d\n", ret);

    /*
     * should subtract 10 bytes from real response size:
     *      2 bytes for tag
     *      4 bytes for size
     *      4 bytes for return code
     */
    rsp_size -= (rsp_size > RSP_HEAD_SIZE) ? RSP_HEAD_SIZE : rsp_size;

    if ( ret != TPM_SUCCESS )
        return ret;

    if ( *out_size == 0 || rsp_size == 0 )
        *out_size = 0;
    else
        *out_size = (rsp_size < *out_size) ? rsp_size : *out_size;

    return ret;
}

static inline uint32_t tpm_submit_cmd(uint32_t cmd, uint32_t arg_size, uint32_t *out_size)
{
   return  _tpm_submit_cmd(TPM_TAG_RQU_COMMAND, cmd, arg_size, out_size);
}

uint32_t tpm_pcr_extend(uint32_t pcr, const tpm_digest_t* in, tpm_pcr_value_t* out)
{
    uint32_t ret, in_size = 0, out_size;

	fprintf(STDOUT, "Extend to pcr %d\n", pcr);
	PrintBytes("in-digest:", in->digest, 20);

    if ( in == NULL )
        return TPM_BAD_PARAMETER;
    if ( pcr >= TPM_NR_PCRS )
        return TPM_BAD_PARAMETER;
    if ( out == NULL )
        out_size = 0;
    else
        out_size = sizeof(*out);

    /* copy pcr into buf in reversed byte order, then copy in data */
    reverse_copy(WRAPPER_IN_BUF, &pcr, sizeof(pcr));
    in_size += sizeof(pcr);
    memcpy_s(WRAPPER_IN_BUF + in_size, WRAPPER_IN_MAX_SIZE-in_size, (void *)in, sizeof(*in));
    in_size += sizeof(*in);

    ret = tpm_submit_cmd(TPM_ORD_PCR_EXTEND, in_size, &out_size);

#ifdef TPM_TRACE
    fprintf(STDOUT,"TPM: Pcr %d Extend, return value = %08X\n", pcr, ret);
#endif
    if ( ret != TPM_SUCCESS ) {
        fprintf(STDOUT, "TPM: Pcr %d Extend not successful, return value = %08X\n", pcr, ret);
        return ret;
    }

    if ( out != NULL && out_size > 0 ) {
       out_size = (out_size > sizeof(*out)) ? sizeof(*out) : out_size;
       memcpy_s((void *)out, out_size, WRAPPER_OUT_BUF, out_size);
    }

	PrintBytes("TPM after extension, out:", out->digest, 20);

#ifdef TPM_TRACE
    {
        fprintf(STDOUT, "TPM: ");
        print_hex(NULL, out->digest, out_size);
    }
#endif

    return ret;
}

uint32_t tpm_pcr_read(uint32_t pcr, tpm_pcr_value_t *out)
{
    uint32_t ret, out_size = sizeof(*out);

    if ( out == NULL )
        return TPM_BAD_PARAMETER;
    if ( pcr >= TPM_NR_PCRS )
        return TPM_BAD_PARAMETER;

    /* copy pcr into buf in reversed byte order */
    reverse_copy(WRAPPER_IN_BUF, &pcr, sizeof(pcr));

    ret = tpm_submit_cmd(TPM_ORD_PCR_READ, sizeof(pcr), &out_size);

#ifdef TPM_TRACE
    fprintf(STDOUT, "TPM: Pcr %d Read, return value = %08X\n", pcr, ret);
#endif
    if ( ret != TPM_SUCCESS ) {
        fprintf(STDOUT, "TPM: Pcr %d Read not successful, return value = %08X\n", pcr, ret);
        return ret;
    }

    if ( out_size > sizeof(*out) )
        out_size = sizeof(*out);

    memcpy_s((void *)out, out_size, WRAPPER_OUT_BUF, out_size);

#ifdef TPM_TRACE
    {
        fprintf(STDOUT, "TPM: ");
        print_hex(NULL, out->digest, out_size);
    }
#endif

    return ret;
}
