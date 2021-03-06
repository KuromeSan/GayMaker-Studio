// DotsTheBee.cpp : Defines the exported functions for the DLL application.
//
// https://github.com/dots-tb/scever_patch

#include "stdafx.h"
#include "elf.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>


extern "C" __declspec(dllexport) int SceVerDown(char *file, uint32_t version)
{
	uint32_t fw_num = version;
	printf("Setting to fw to: %x\n", fw_num);
	printf("Opening: %s\n", file);
	FILE *fp = fopen(file, "r+b");
	if (!fp)
		return (int)fp;
	Elf64_Ehdr ehdr;
	fread(&ehdr, sizeof(ehdr), 1, fp);

	printf("Checking if ELF...\n");
	char elf_magic[8] = { 0x7F, 0x45, 0x4C, 0x46, 0x02, 0x01, 0x01, 0x09 };

	if (memcmp(&ehdr.e_ident, &elf_magic, 8))
	{
		fclose(fp);
		return 0x0;
	}

	Elf64_Phdr *phdrs = (Elf64_Phdr*)malloc(ehdr.e_phentsize * ehdr.e_phnum);
	fseek(fp, ehdr.e_phoff, SEEK_SET);
	fread(phdrs, ehdr.e_phentsize * ehdr.e_phnum, 1, fp);
	for (int i = 0; i < ehdr.e_phnum; ++i) {
		if (phdrs[i].p_type == 0x61000002 || phdrs[i].p_type == 0x61000001) {
			printf("sce_process_param patch\n");
			char *sce_process_param = (char *)malloc(phdrs[i].p_filesz);
			fseek(fp, phdrs[i].p_offset, SEEK_SET);
			fread(sce_process_param,phdrs[i].p_filesz,1,fp);
			uint32_t *req_ver = (uint32_t *)((char*)sce_process_param + 0x10);
			printf("Original version: %x\n", *req_ver); 
			*req_ver = fw_num;
			printf("Writing version: %x\n", *req_ver);
			fseek(fp, phdrs[i].p_offset, SEEK_SET);
			fwrite(sce_process_param,phdrs[i].p_filesz,1,fp);
			free(sce_process_param);		
		}
		if (phdrs[i].p_type == 0x6fffff01) {
			printf("sce_version patch\n");
			char *sce_version = (char*)malloc(phdrs[i].p_filesz);
			fseek(fp, phdrs[i].p_offset, SEEK_SET);
			fread(sce_version, phdrs[i].p_filesz, 1, fp);

			size_t offset = 0;
			while (offset < phdrs[i].p_filesz) {
				int sz = *((char *)sce_version + offset);
				char name[128] = "";
				memset(name, 0x00, 128);
				memcpy(name, sce_version + offset + 1, sz - 0x4);
				printf("Library name: %s sz: %x\n", name,sz -0x4);
				uint32_t *req_ver = (uint32_t *)((char*)sce_version + offset + 1 + sz - 0x4);
				printf("Original version: %x\n", _byteswap_ulong(*req_ver));
				*req_ver = _byteswap_ulong(fw_num);
				printf("Setting version: %x\n", _byteswap_ulong(*req_ver));
				offset += sz + 1;
			}


			printf("Writing sce_version...\n");
			fseek(fp, phdrs[i].p_offset, SEEK_SET);
			fwrite(sce_version, phdrs[i].p_filesz, 1, fp);
			free(sce_version);
			fclose(fp);
		}


	}

	return 1;
}



