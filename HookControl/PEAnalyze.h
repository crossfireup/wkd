#ifndef _PEANALYZE_H
#define _PEANALYZE_H

namespace win {
	class PEImg {
	public:
		PEImg(HMODULE hModule) :hModule_{ hModule }{};
		virtual ~PEImg(){};

		HMODULE GetModule() const;
		void SetModule(HMODULE hModule);

		PIMAGE_DOS_HEADER GetDosHeader() const;

		PIMAGE_NT_HEADERS GetNTHeaders() const;

		WORD GetSectionNum() const;

		PIMAGE_SECTION_HEADER GetFirstSection() const;

		PIMAGE_SECTION_HEADER GetSectionHeader(UINT section) const;

		PIMAGE_SECTION_HEADER GetImageSectionFromAddr(PVOID addr) const;

		DWORD GetImageDirectoryEntrySize(UINT directory) const;

		PVOID GetImageDirectoryEntryAddr(UINT directory) const;

		PIMAGE_IMPORT_DESCRIPTOR GetFirstImportChunk() const;

		PIMAGE_EXPORT_DIRECTORY GetExportDirectory() const;

		virtual PVOID RVAToAddr(DWORD rva) const;

		bool ImgAddrToOnDiskOffset(DWORD rva, DWORD *onDiskOffset) const;

		bool ImgAddrToOnDiskOffset(PVOID addr, DWORD *onDiskOffset) const;

	private:
		HMODULE hModule_;
	};

	class PEImgAsData : public PEImg{
	public:
		explicit PEImgAsData(HMODULE hModule) :PEImg(hModule){};

		PVOID RVAToAddr(DWORD rva) const override;
	};

};

#endif /* _PEANALYZE_H */