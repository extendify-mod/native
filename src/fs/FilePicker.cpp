#include "FilePicker.hpp"

#include "api/util/ScopedV8Context.hpp"
#include "log/log.hpp"
#include "log/Logger.hpp"
#include "main.hpp"
#include "util/string.hpp"
#include "util/TaskCBHandler.hpp"

#include <cef_task.h>
#include <concepts>
#include <internal/cef_types.h>
#include <memory>
#include <winnt.h>

#ifdef _WIN32
#include <windows.h>
#define STRICT_TYPED_ITEMIDS
#include <shlwapi.h>
#include <shobjidl.h>

#endif

#ifdef _WIN32
const static Extendify::log::Logger logger {{"Extendify", "api", "FilePicker"}};
namespace Extendify::fs {

	namespace {
		class CDialogEventHandler:
			public IFileDialogEvents,
			public IFileDialogControlEvents {
		  public:
			// IUnknown methods
			IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
				static const QITAB qit[] = {
					QITABENT(CDialogEventHandler, IFileDialogEvents),
					QITABENT(CDialogEventHandler, IFileDialogControlEvents),
					{0},
#pragma warning(suppress : 4838)
				};
				return QISearch(this, qit, riid, ppv);
			}

			IFACEMETHODIMP_(ULONG) AddRef() override {
				return InterlockedIncrement(&_cRef);
			}

			IFACEMETHODIMP_(ULONG) Release() override {
				long cRef = InterlockedDecrement(&_cRef);
				if (!cRef)
					delete this;
				return cRef;
			}

			// IFileDialogEvents methods
			IFACEMETHODIMP OnFileOk(IFileDialog*) override {
				return S_OK;
			};

			IFACEMETHODIMP OnFolderChange(IFileDialog*) override {
				return S_OK;
			};

			IFACEMETHODIMP OnFolderChanging(IFileDialog*,
											IShellItem*) override {
				return S_OK;
			};

			IFACEMETHODIMP OnHelp(IFileDialog*) {
				return S_OK;
			};

			IFACEMETHODIMP OnSelectionChange(IFileDialog*) override {
				return S_OK;
			};

			IFACEMETHODIMP
			OnShareViolation(IFileDialog*, IShellItem*,
							 FDE_SHAREVIOLATION_RESPONSE*) override {
				return S_OK;
			};

			IFACEMETHODIMP OnTypeChange(IFileDialog* pfd) override;

			IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*,
									   FDE_OVERWRITE_RESPONSE*) override {
				return S_OK;
			};

			// IFileDialogControlEvents methods
			IFACEMETHODIMP OnItemSelected(IFileDialogCustomize* pfdc,
										  DWORD dwIDCtl,
										  DWORD dwIDItem) override;

			IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*,
										   DWORD) override {
				return S_OK;
			};

			IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD,
												::BOOL) override {
				return S_OK;
			};

			IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*,
											   DWORD) override {
				return S_OK;
			};

			CDialogEventHandler():
				_cRef(1) { };

			static HRESULT CreateInstance(REFIID riid, void** ppv);

		  private:
			~CDialogEventHandler() { };
			long _cRef;
			static constexpr int INDEX_WORDDOC = 1;
			static constexpr int INDEX_WEBPAGE = 2;
			static constexpr int INDEX_TEXTDOC = 3;
			// Controls
			static constexpr int CONTROL_GROUP = 2000;
			static constexpr int CONTROL_RADIOBUTTONLIST = 2;
			static constexpr int CONTROL_RADIOBUTTON1 = 1;
			static constexpr int CONTROL_RADIOBUTTON2 =
				2; // It is OK for this to have the same ID as
				   // CONTROL_RADIOBUTTONLIST, because it is a child control
				   // under CONTROL_RADIOBUTTONLIST
		};

		// IFileDialogEvents methods
		// This method gets called when the file-type is changed (combo-box
		// selection changes). For sample sake, let's react to this event by
		// changing the properties show.
		HRESULT CDialogEventHandler::OnTypeChange(IFileDialog* pfd) {
			IFileSaveDialog* pfsd;
			HRESULT hr = pfd->QueryInterface(&pfsd);
			if (SUCCEEDED(hr)) {
				::UINT uIndex;
				hr = pfsd->GetFileTypeIndex(
					&uIndex); // index of current file-type
				if (SUCCEEDED(hr)) {
					IPropertyDescriptionList* pdl = NULL;

					// NOLINTNEXTLINE(hicpp-multiway-paths-covered) ms code
					switch (uIndex) {
						case INDEX_WORDDOC:
							// When .doc is selected, let's ask for some
							// arbitrary property, say Title.
							hr = PSGetPropertyDescriptionListFromString(
								L"prop:System.Title", IID_PPV_ARGS(&pdl));
							if (SUCCEEDED(hr)) {
								// FALSE as second param == do not show default
								// properties.
								hr = pfsd->SetCollectedProperties(pdl, FALSE);
								pdl->Release();
							}
							break;

						case INDEX_WEBPAGE:
							// When .html is selected, let's ask for some other
							// arbitrary property, say Keywords.
							hr = PSGetPropertyDescriptionListFromString(
								L"prop:System.Keywords", IID_PPV_ARGS(&pdl));
							if (SUCCEEDED(hr)) {
								// FALSE as second param == do not show default
								// properties.
								hr = pfsd->SetCollectedProperties(pdl, FALSE);
								pdl->Release();
							}
							break;

						case INDEX_TEXTDOC:
							// When .txt is selected, let's ask for some other
							// arbitrary property, say Author.
							hr = PSGetPropertyDescriptionListFromString(
								L"prop:System.Author", IID_PPV_ARGS(&pdl));
							if (SUCCEEDED(hr)) {
								// TRUE as second param == show default
								// properties as well, but show Author property
								// first in list.
								hr = pfsd->SetCollectedProperties(pdl, TRUE);
								pdl->Release();
							}
							break;
					}
				}
				pfsd->Release();
			}
			return hr;
		}

		// IFileDialogControlEvents
		// This method gets called when an dialog control item selection happens
		// (radio-button selection. etc). For sample sake, let's react to this
		// event by changing the dialog title.
		HRESULT CDialogEventHandler::OnItemSelected(IFileDialogCustomize* pfdc,
													DWORD dwIDCtl,
													DWORD dwIDItem) {
			IFileDialog* pfd = nullptr;
			HRESULT hr = pfdc->QueryInterface(&pfd);
			if (SUCCEEDED(hr)) {
				if (dwIDCtl == CONTROL_RADIOBUTTONLIST) {
					// NOLINTNEXTLINE(hicpp-multiway-paths-covered) ms code
					switch (dwIDItem) {
						case CONTROL_RADIOBUTTON1:
							hr = pfd->SetTitle(L"Longhorn Dialog");
							break;

						case CONTROL_RADIOBUTTON2:
							hr = pfd->SetTitle(L"Vista Dialog");
							break;
					}
				}
				pfd->Release();
			}
			return hr;
		}

		// Instance creation helper
		HRESULT CDialogEventHandler::CreateInstance(REFIID riid, void** ppv) {
			*ppv = nullptr;
			auto* pDialogEventHandler =
				new (std::nothrow) CDialogEventHandler();
			HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
			if (SUCCEEDED(hr)) {
				hr = pDialogEventHandler->QueryInterface(riid, ppv);
				pDialogEventHandler->Release();
			}
			return hr;
		}

		const char* errStrDialog(HRESULT code) {
			switch (code) {
				case REGDB_E_CLASSNOTREG: {
					return "A specified class is not registered in the "
						   "registration database. Also can indicate that the "
						   "type of server you requested in the CLSCTX "
						   "enumeration is not registered or the values for "
						   "the "
						   "server types in the registry are corrupt.";
				}
				case CLASS_E_NOAGGREGATION: {
					return "This class cannot be created as part of an "
						   "aggregate.";
				}
				case E_NOINTERFACE: {
					return "The specified class does not implement the "
						   "requested interface, or the controlling "
						   "IUnknown "
						   "does not expose the requested interface.";
				}
				case E_POINTER: {
					return "The ppv parameter is NULL.";
				}
				default: {
					return "Unknown error";
				}
			}
		};

		class IErrorCode {
		  public:
			HRESULT code {};
		};

		template<typename T>
		requires std::derived_from<T, IUnknown>
		struct w_ptr_t: public std::unique_ptr<T, decltype([](T* ptr) {
												   if (ptr) {
													   ptr->Release();
												   }
											   })> { };

		class w_FileDialogEvents:
			public w_ptr_t<IFileDialogEvents>,
			public IErrorCode {
		  public:
			constexpr static w_FileDialogEvents Create() {
				w_FileDialogEvents events;
				IFileDialogEvents* tmp = nullptr;
				events.code =
					CDialogEventHandler::CreateInstance(IID_PPV_ARGS(&tmp));
				events.reset(tmp);
				return std::move(events);
			}
		};

		class Iw_ShellItem: public IErrorCode {
		  public:
			[[nodiscard]] virtual std::vector<std::filesystem::path>
			getItems() = 0;
			virtual ~Iw_ShellItem() = default;
		};

		class Iw_FileDialog: public IErrorCode {
		  public:
			virtual HRESULT advise(w_FileDialogEvents& pfde,
								   DWORD* pdwCookie) = 0;
			virtual HRESULT advise(IFileDialogEvents* pfde,
								   DWORD* pdwCookie) = 0;
			[[nodiscard]] virtual std::unique_ptr<Iw_ShellItem> getResult() = 0;
			virtual HRESULT getOptions(DWORD* pFlags) = 0;
			virtual HRESULT setOptions(DWORD flags) = 0;
			/// {{name, extension(s)}}
			virtual HRESULT
			setFileTypes(const FilePicker::FilePickerData& data) = 0;
			virtual HRESULT setDefaultExtension(const std::wstring& ext) = 0;
			virtual HRESULT setTitle(std::string title) = 0;
			virtual HRESULT setStateId(const ids::ExtendifyId& id) = 0;
			virtual HRESULT
			setDefaultFolder(const std::filesystem::path& path) = 0;
			virtual HRESULT show() = 0;
			virtual ~Iw_FileDialog() = default;

		  protected:
			std::vector<std::wstring> dialogStrings;
			std::wstring title;
			w_ptr_t<IShellItem> defaultFolder;

			HRESULT _setDefaultFolderItem(const std::filesystem::path& path) {
				IShellItem* tmp = nullptr;
				auto ret = SHCreateItemFromParsingName(
					path.c_str(), nullptr, IID_PPV_ARGS(&tmp));
				defaultFolder.reset(tmp);
				return ret;
			}

			COMDLG_FILTERSPEC
			transformFilter(const FilePicker::FileFilter& filter) {
				constexpr const static auto defaultExt = L"*.*";
				constexpr const static auto defaultDisplayName =
					L"Unknown filter";
				COMDLG_FILTERSPEC spec;
				if (filter.displayName.empty()) {
					logger.warn("FileFilter has no display name, using "
								"'Unknown filter'");
					spec.pszName = defaultDisplayName;
				} else {
					auto str =
						util::string::stringToWstring(filter.displayName);
					dialogStrings.emplace_back(std::move(str));
					spec.pszName = dialogStrings.back().c_str();
				}
				if (filter.patterns.empty()) {
					logger.warn(
						"FileFilter has no patterns, using default *.*");
					spec.pszSpec = defaultExt;
				} else {
					auto str = util::string::stringToWstring(
						util::string::join(filter.patterns, ";"));
					dialogStrings.emplace_back(std::move(str));
					spec.pszSpec = dialogStrings.back().c_str();
				}

				return spec;
			}
		};

		class w_ShellItem: public w_ptr_t<IShellItem>, public Iw_ShellItem {
		  public:
			[[nodiscard]] std::vector<std::filesystem::path>
			getItems() override {
				return {itemToPath(this->get())};
			}

		  private:
			[[nodiscard]] static std::filesystem::path
			itemToPath(IShellItem* item) {
				LPWSTR pszName = nullptr;
				auto hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszName);
				if (FAILED(hr) || !pszName) {
					logger.error(
						"Failed to get display name of shell item: code: "
						"{}, msg: {}",
						hr,
						errStrDialog(hr));
					return {};
				}
				std::filesystem::path path(pszName);
				CoTaskMemFree(pszName);
				return std::move(path);
			}
			friend class w_ShellItems;
		};

		class w_ShellItems:
			public w_ptr_t<IShellItemArray>,
			public Iw_ShellItem {
		  public:
			[[nodiscard]] std::vector<std::filesystem::path>
			getItems() override {
				E_ASSERT(this->get() && "ShellItems is not initialized");
				std::vector<std::filesystem::path> items;
				DWORD size = -1;
				HRESULT hr = this->get()->GetCount(&size);
				if (FAILED(hr)) {
					logger.error(
						"Failed to get count of shell items: code: {}, msg: {}",
						hr,
						errStrDialog(hr));
					return items;
				}
				assert(size >= 0 && "ShellItems count is negative");
				items.reserve(size);
				IShellItem* item = nullptr;
				for (auto i = 0; i < size; i++) {
					hr = get()->GetItemAt(i, &item);
					if (FAILED(hr)) {
						logger.error(
							"Failed to get shell item at index {}: code: "
							"{}, msg: {}",
							i,
							hr,
							errStrDialog(hr));
						continue;
					}
					E_ASSERT(item && "ShellItem is null");
					items.push_back(w_ShellItem::itemToPath(item));
				}
				return std::move(items);
			}
		};

		class w_FileOpenDialog:
			public w_ptr_t<IFileOpenDialog>,
			public Iw_FileDialog {

		  public:
			w_FileOpenDialog():
				w_ptr_t<IFileOpenDialog>(nullptr) {
			}

			w_FileOpenDialog(const w_FileOpenDialog& other) = delete;
			w_FileOpenDialog& operator=(const w_FileOpenDialog& other) = delete;

			w_FileOpenDialog(w_FileOpenDialog&& other) noexcept:
				w_ptr_t<IFileOpenDialog>(std::move(other)),
				advCookies(std::move(other.advCookies)) {
			}

			w_FileOpenDialog& operator=(w_FileOpenDialog&& other) noexcept {
				if (this != &other) {
					w_ptr_t<IFileOpenDialog>::operator=(std::move(other));
					advCookies = std::move(other.advCookies);
				}
				return *this;
			}

			~w_FileOpenDialog() {
				for (auto& cookie : advCookies) {
					get()->Unadvise(*cookie);
				}
			}

			constexpr static std::unique_ptr<w_FileOpenDialog> Create() {
				auto dialog = std::make_unique<w_FileOpenDialog>();
				IFileOpenDialog* tmp = nullptr;
				dialog->code = CoCreateInstance(CLSID_FileOpenDialog,
												nullptr,
												CLSCTX_INPROC_SERVER,
												IID_PPV_ARGS(&tmp));
				dialog->reset(tmp);
				return dialog;
			}

			HRESULT advise(w_FileDialogEvents& pfde,
						   DWORD* pdwCookie) override {
				return advise(pfde.get(), pdwCookie);
			}

			HRESULT advise(IFileDialogEvents* pfde, DWORD* pdwCookie) override {
				E_ASSERT(pfde && "pfde is null");
				E_ASSERT(pdwCookie && "pdwCookie is null");
				E_ASSERT(this->get() && "FileDialog is not initialized");
				advCookies.insert(pdwCookie);
				return this->get()->Advise(pfde, pdwCookie);
			}

			[[nodiscard]] std::unique_ptr<Iw_ShellItem> getResult() override {
				E_ASSERT(this->get() && "FileDialog is not initialized");
				auto ret = std::make_unique<w_ShellItems>();
				IShellItemArray* tmp = nullptr;
				ret->code = this->get()->GetResults(&tmp);
				ret->reset(tmp);
				return ret;
			}

			HRESULT getOptions(DWORD* pFlags) override {
				E_ASSERT(pFlags && "pFlags is null");
				E_ASSERT(this->get() && "FileDialog is not initialized");
				return this->get()->GetOptions(pFlags);
			}

			HRESULT setOptions(DWORD flags) override {
				E_ASSERT(this->get() && "FileDialog is not initialized");
				return this->get()->SetOptions(flags);
			}

			HRESULT
			setFileTypes(const FilePicker::FilePickerData& fileTypes) override {
				// doesnt change the capacity
				dialogStrings.clear();
				// each filter has a name and a patter, + 2 for default filter
				dialogStrings.reserve(fileTypes.filters.size() * 2 + 2);

				filters.clear();
				filters.reserve(fileTypes.filters.size() + 1);

				filters.push_back(transformFilter(fileTypes.defaultFilter));
				for (const auto& filter : fileTypes.filters) {
					filters.push_back(transformFilter(filter));
				}

				E_ASSERT(get() && "FileDialog is not initialized");
				const auto hr =
					get()->SetFileTypes(filters.size(), filters.data());
				if (FAILED(hr)) {
					return hr;
				}
				// we pass a 0-based array, but set the index as 1-based
				// more:
				// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifiledialog-setfiletypeindex#parameters
				return get()->SetFileTypeIndex(1);
			}

			HRESULT setDefaultExtension(const std::wstring& ext) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				return get()->SetDefaultExtension(ext.c_str());
			}

			HRESULT setTitle(std::string _title) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				if (_title.empty()) {
					title = L"Open File(s)";
				} else {
					title = std::move(
						util::string::stringToWstring(std::move(_title)));
				}
				return get()->SetTitle(title.c_str());
			}

			HRESULT setStateId(const ids::ExtendifyId& id) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				E_ASSERT(id && "id is null");
				return get()->SetClientGuid(ids::extendifyIdToGUID(id));
			}

			HRESULT
			setDefaultFolder(const std::filesystem::path& path) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				if (path.empty()) {
					logger.warn("Default folder path is empty, skipping");
					return S_OK;
				}

				auto hr = _setDefaultFolderItem(path);
				if (FAILED(hr)) {
					return hr;
				}
				return get()->SetDefaultFolder(defaultFolder.get());
			}

			HRESULT show() override {
				E_ASSERT(get() && "FileDialog is not initialized");
				return get()->Show(nullptr);
			}

		  private:
			std::unordered_set<DWORD*> advCookies;
			std::vector<COMDLG_FILTERSPEC> filters;
		};

		class w_FileSaveDialog:
			public w_ptr_t<IFileSaveDialog>,
			public Iw_FileDialog {
		  public:
			w_FileSaveDialog():
				w_ptr_t<IFileSaveDialog>(nullptr) {
			}

			w_FileSaveDialog(const w_FileSaveDialog& other) = delete;
			w_FileSaveDialog& operator=(const w_FileSaveDialog& other) = delete;

			w_FileSaveDialog(w_FileSaveDialog&& other) noexcept:
				w_ptr_t<IFileSaveDialog>(std::move(other)),
				advCookies(std::move(other.advCookies)) {
			}

			w_FileSaveDialog& operator=(w_FileSaveDialog&& other) noexcept {
				if (this != &other) {
					w_ptr_t<IFileSaveDialog>::operator=(std::move(other));
					advCookies = std::move(other.advCookies);
				}
				return *this;
			}

			constexpr static std::unique_ptr<w_FileSaveDialog> Create() {
				auto dialog = std::make_unique<w_FileSaveDialog>();
				IFileSaveDialog* tmp = nullptr;
				dialog->code = CoCreateInstance(CLSID_FileSaveDialog,
												nullptr,
												CLSCTX_INPROC_SERVER,
												IID_PPV_ARGS(&tmp));
				dialog->reset(tmp);
				return dialog;
			}

			HRESULT advise(w_FileDialogEvents& pfde,
						   DWORD* pdwCookie) override {
				return advise(pfde.get(), pdwCookie);
			}

			HRESULT advise(IFileDialogEvents* pfde, DWORD* pdwCookie) override {
				E_ASSERT(pfde && "pfde is null");
				E_ASSERT(pdwCookie && "pdwCookie is null");
				E_ASSERT(this->get() && "FileDialog is not initialized");
				advCookies.insert(pdwCookie);
				return this->get()->Advise(pfde, pdwCookie);
			}

			[[nodiscard]] std::unique_ptr<Iw_ShellItem> getResult() override {
				E_ASSERT(this->get() && "FileSaveDialog is not initialized");
				auto ret = std::make_unique<w_ShellItem>();
				IShellItem* tmp = nullptr;
				ret->code = this->get()->GetResult(&tmp);
				ret->reset(tmp);
				return ret;
			}

			HRESULT getOptions(DWORD* pFlags) override {
				E_ASSERT(pFlags && "pFlags is null");
				E_ASSERT(this->get() && "FileDialog is not initialized");
				return this->get()->GetOptions(pFlags);
			}

			HRESULT setOptions(DWORD flags) override {
				E_ASSERT(this->get() && "FileDialog is not initialized");
				return this->get()->SetOptions(flags);
			}

			HRESULT
			setFileTypes(const FilePicker::FilePickerData& fileTypes) override {
				// doesnt change the capacity
				dialogStrings.clear();
				// each filter has a name and a patter, + 2 for default filter
				dialogStrings.reserve(fileTypes.filters.size() * 2 + 2);

				filters.clear();
				filters.reserve(fileTypes.filters.size() + 1);

				filters.push_back(transformFilter(fileTypes.defaultFilter));
				for (const auto& filter : fileTypes.filters) {
					filters.push_back(transformFilter(filter));
				}

				E_ASSERT(get() && "FileDialog is not initialized");
				const auto hr =
					get()->SetFileTypes(filters.size(), filters.data());
				if (FAILED(hr)) {
					return hr;
				}
				// we pass a 0-based array, but set the index as 1-based
				// more:
				// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifiledialog-setfiletypeindex#parameters
				return get()->SetFileTypeIndex(1);
			}

			HRESULT setDefaultExtension(const std::wstring& ext) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				return get()->SetDefaultExtension(ext.c_str());
			}

			HRESULT setTitle(std::string _title) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				if (_title.empty()) {
					title = L"Save File";
				} else {
					title = std::move(
						util::string::stringToWstring(std::move(_title)));
				}
				return get()->SetTitle(title.c_str());
			}

			HRESULT setStateId(const ids::ExtendifyId& id) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				E_ASSERT(id && "id is null");
				return get()->SetClientGuid(ids::extendifyIdToGUID(id));
			}

			HRESULT
			setDefaultFolder(const std::filesystem::path& path) override {
				E_ASSERT(get() && "FileDialog is not initialized");
				if (path.empty()) {
					logger.warn("Default folder path is empty, skipping");
					return S_OK;
				}

				auto hr = _setDefaultFolderItem(path);
				if (FAILED(hr)) {
					return hr;
				}
				return get()->SetDefaultFolder(defaultFolder.get());
			}

			HRESULT show() override {
				E_ASSERT(get() && "FileDialog is not initialized");
				return get()->Show(nullptr);
			}

		  private:
			std::unordered_set<DWORD*> advCookies;
			std::vector<COMDLG_FILTERSPEC> filters;
		};

	} // namespace
#endif

	int FilePicker::nextId = 1;

	FilePicker::FilePicker(FilePickerData data):
		data(std::move(data)) {
	}

	[[nodiscard]] CefRefPtr<FilePicker>
	FilePicker::Create(FilePickerData data) {
		CefRefPtr<FilePicker> ret = new FilePicker(std::move(data));
		ret->self = ret;
		return ret;
	}

	// NOLINTNEXTLINE(performance-unnecessary-value-param)
	[[nodiscard]] CefRefPtr<CefV8Value>
	FilePicker::promise(CefRefPtr<CefV8Context> _context,
						PromiseCallback _callback) {
		if (isRunning()) {
			constexpr auto msg =
				"Trying to launch a file picker while another is "
				"already running for the same object";
			logger.error(msg);
			throw std::runtime_error(msg);
		}
		running = true;
		api::util::ScopedV8Context ctx(_context);
		auto promise = CefV8Value::CreatePromise();
		callbackData = V8Data {
			.context = _context,
			.promise = promise,
			.callback = std::move(_callback),
		};
		runFilePicker();
		return promise;
	}

	void FilePicker::launch(Callback callback) {
		if (isRunning()) {
			constexpr auto msg =
				"Trying to launch a file picker while another is "
				"already running for the same object";
			logger.error(msg);
			throw std::runtime_error(msg);
		}
		running = true;
		callbackData = RawData {std::move(callback)};
		runFilePicker();
	}

	void FilePicker::runFilePicker() {
		pickerThread =
			CefThread::CreateThread(std::format("FilePicker-{}", nextId++));
		pickerThread->GetTaskRunner()->PostTask(
			util::TaskCBHandler::Create([this]() {
				auto res = showDialog();
				std::vector<std::filesystem::path> paths;
				std::optional<std::string> err;
				if (res.has_value()) {
					paths = std::move(res.value());
				} else {
					err = std::move(res.error());
				}
				if (std::holds_alternative<V8Data>(callbackData)) {
					CefTaskRunner::GetForThread(TID_RENDERER)
						->PostTask(util::TaskCBHandler::Create(
							[this,
							 paths = std::move(paths),
							 err = std::move(err)]() {
								auto& v8Data = std::get<V8Data>(callbackData);
								E_ASSERT(v8Data.context->IsValid()
										 && "V8Context is not valid");
								v8Data.context->GetTaskRunner()->PostTask(
									util::TaskCBHandler::Create([=,
																 &v8Data,
																 this]() {
										api::util::ScopedV8Context ctx(
											v8Data.context);
										// keep a ref so dont drop it if the
										// callback drops it
										auto _ = self;
										std::expected<CefRefPtr<CefV8Value>,
													  std::string>
											ret;
										try {
											ret = v8Data.callback(
												std::move(self), paths, err);
										} catch (std::exception& e) {
											const auto msg = std::format(
												"Error in FilePicker promise "
												"callback: {}",
												e.what());
											logger.error(msg);
											ret = std::unexpected(
												std::string(e.what()));
										}
										try {
											if (ret.has_value()) {
												v8Data.promise->ResolvePromise(
													std::move(ret.value()));
											} else {
												v8Data.promise->RejectPromise(
													ret.error());
											}
										} catch (const std::exception& e) {
											logger.error(
												"ERROR: at {}:{}, what: {}",
												__FILE__,
												__LINE__,
												e.what());
										}
										running = false;
									}));
							}));
				} else {
					auto& rawData = std::get<RawData>(this->callbackData);
					// keep a ref so dont drop it if the callback
					auto _ = self;
					try {
						rawData.callback(
							std::move(self), std::move(paths), std::move(err));
					} catch (const std::exception& e) {
						logger.error("Error in FilePicker callback: {}",
									 e.what());
					}
					running = false;
				}
			}));
	}

	[[nodiscard]] std::expected<std::vector<std::filesystem::path>, std::string>
	FilePicker::showDialog() const {
#ifdef _WIN32
#ifdef E_CHECK_ERR
#warning "E_CHECK_ERR is already defined, redefining it"
#endif
#define E_CHECK_ERR(hr)                                                \
	if (FAILED(hr)) {                                                  \
		const auto msg = std::format("Error in {}. code: {}, msg: {}", \
									 __PRETTY_FUNCTION__,              \
									 hr,                               \
									 errStrDialog(hr));                \
		logger.error(msg);                                             \
		return std::unexpected(msg);                                   \
	}
		auto isSave = data.mode == DialogType::SAVE;
		// CoCreate the File Open Dialog object.
		std::unique_ptr<Iw_FileDialog> fileDialog;
		if (isSave) {
			fileDialog = w_FileSaveDialog::Create();
		} else {
			fileDialog = w_FileOpenDialog::Create();
		}

		E_CHECK_ERR(fileDialog->code);
		// Create an event handling object, and hook it up to the dialog.
		auto eventsHandler = w_FileDialogEvents::Create();
		E_CHECK_ERR(eventsHandler.code);
		DWORD dwCookie;
		// Hook up the event handler.
		HRESULT hr = fileDialog->advise(eventsHandler, &dwCookie);
		E_CHECK_ERR(hr);
		// Set the options on the dialog.
		DWORD dwFlags;
		// Before setting, always get the options first in order
		// not to override existing options.
		hr = fileDialog->getOptions(&dwFlags);
		E_CHECK_ERR(hr);
		// In this case, get shell items only for file system items.
		{
			DWORD flags = dwFlags | FOS_FORCEFILESYSTEM;
			flags |= FOS_OKBUTTONNEEDSINTERACTION;
			switch (data.mode) {
				case DialogType::OPEN: {
					break;
				}
				case DialogType::OPEN_MANY: {
					flags |= FOS_ALLOWMULTISELECT;
					break;
				}
				case DialogType::OPEN_FOLDER: {
					flags |= FOS_PICKFOLDERS;
					break;
				}
				case DialogType::SAVE: {
					flags &= ~FOS_FILEMUSTEXIST;
				}
				default: {
					E_ASSERT(false && "unhandled DialogType");
				}
			}
			hr = fileDialog->setOptions(flags);
		}
		E_CHECK_ERR(hr);
		// set the default file type and file types
		hr = fileDialog->setFileTypes(data);
		E_CHECK_ERR(hr);
		// set the title
		hr = fileDialog->setTitle(data.title);
		E_CHECK_ERR(hr);
		// set the state id
		hr = fileDialog->setStateId(data.stateId);
		E_CHECK_ERR(hr);
		// set the default folder
		hr = fileDialog->setDefaultFolder(data.defaultFolderPath);
		E_CHECK_ERR(hr);
		// // Set the default extension to be ".css" file.
		// hr = fileDialog->setDefaultExtension(L"css");
		// E_CHECK_ERR(hr);
		// Show the dialog
		hr = fileDialog->show();
		E_CHECK_ERR(hr);
		// Obtain the result once the user clicks
		// the 'Open' button.
		// The result is an IShellItem object.
		auto result = fileDialog->getResult();
		E_CHECK_ERR(result->code);
		// get the file path
		auto paths = result->getItems();
		return paths;

#undef E_CHECK_ERR
#endif
	}

	[[nodiscard]] CefRefPtr<FilePicker> FilePicker::pickOne() {
		return FilePicker::Create(FilePickerData {
			.mode = DialogType::OPEN,
		});
	}

	[[nodiscard]] CefRefPtr<FilePicker> FilePicker::pickMany() {
		return FilePicker::Create(FilePickerData {
			.mode = DialogType::OPEN_MANY,
		});
	}

	[[nodiscard]] CefRefPtr<FilePicker> FilePicker::pickFolder() {
		return FilePicker::Create(FilePickerData {
			.mode = DialogType::OPEN_FOLDER,
		});
	}

	[[nodiscard]] CefRefPtr<FilePicker> FilePicker::pickSaveFile() {
		return FilePicker::Create(FilePickerData {
			.mode = DialogType::SAVE,
		});
	}
}; // namespace Extendify::fs
