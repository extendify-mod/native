#include "api.hpp"

#include "log/log.hpp"
#include "quickCss.hpp"
#include "settings.hpp"
#include "themes.hpp"
#include "util/iter.hpp"
#include "util/string.hpp"
#include "util/TaskCBHandler.hpp"

#include <cef_v8.h>
#include <format>
#include <include/base/cef_scoped_refptr.h>
#include <include/cef_process_message.h>
#include <include/internal/cef_ptr.h>
#include <include/internal/cef_types.h>
#include <utility>
#ifdef _WIN32
#include <windows.h> // For common windows data types and function headers
#define STRICT_TYPED_ITEMIDS
#include <knownfolders.h> // for KnownFolder APIs/datatypes/function headers
#include <new>
#include <objbase.h> // For COM headers
#include <propidl.h> // for the Property System APIs
#include <propkey.h> // for the Property key APIs/datatypes
#include <propvarutil.h> // for PROPVAR-related functions
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h> // for IFileDialogEvents and IFileDialogControlEvents
#include <shtypes.h> // for COMDLG_FILTERSPEC
#include <strsafe.h> // for StringCchPrintfW
#include <winerror.h>
#endif

namespace Extendify::api {
	log::Logger logger({"Extendify", "api"});

	void inject(const CefRefPtr<CefV8Context>& context) {
		logger.info("Injecting ExtendifyNative API into V8 context");
		CefRefPtr<CefV8Value> global = context->GetGlobal();
		CefRefPtr<CefV8Value> extendify =
			CefV8Value::CreateObject(nullptr, nullptr);

		extendify->SetValue(
			"settings", settings::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		extendify->SetValue(
			"quickCss", quickCss::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		extendify->SetValue(
			"themes", themes::makeApi(), V8_PROPERTY_ATTRIBUTE_NONE);

		global->SetValue(
			"ExtendifyNative", extendify, V8_PROPERTY_ATTRIBUTE_NONE);
		global->SetValue("EXTENDIFY_NATIVE_AVAILABLE",
						 CefV8Value::CreateBool(true),
						 V8_PROPERTY_ATTRIBUTE_NONE);
	}

	[[nodiscard]] V8Type getV8Type(const CefRefPtr<CefV8Value>& value) {
		E_ASSERT(value->IsValid() && "V8 value is not valid");

		if (value->IsUndefined()) {
			return V8Type::UNDEFINED;
		} else if (value->IsNull()) {
			return V8Type::NULL_TYPE;
		} else if (value->IsBool()) {
			return V8Type::BOOL;
		} else if (value->IsInt()) {
			return V8Type::INT;
		} else if (value->IsUInt()) {
			return V8Type::UINT;
		} else if (value->IsDouble()) {
			return V8Type::DOUBLE;
		} else if (value->IsDate()) {
			return V8Type::DATE;
		} else if (value->IsString()) {
			return V8Type::STRING;
		} else if (value->IsArray()) {
			return V8Type::ARRAY;
		} else if (value->IsArrayBuffer()) {
			return V8Type::ARRAY_BUFFER;
		} else if (value->IsFunction()) {
			return V8Type::FUNCTION;
		} else if (value->IsPromise()) {
			return V8Type::PROMISE;
		} else if (value->IsObject()) {
			return V8Type::OBJECT;
		}
		E_ASSERT(false && "Unknown V8 value type");
	};

	[[nodiscard]] std::string getTypeName(const CefRefPtr<CefV8Value>& value) {
		return getTypeName(getV8Type(value));
	};

	[[nodiscard]] constexpr std::string getTypeName(V8Type type) {
		switch (type) {
			case V8Type::UNDEFINED:
				return "undefined";
			case V8Type::NULL_TYPE:
				return "null";
			case V8Type::BOOL:
				return "bool";
			case V8Type::INT:
				return "int";
			case V8Type::UINT:
				return "uint";
			case V8Type::DOUBLE:
				return "double";
			case V8Type::DATE:
				return "date";
			case V8Type::STRING:
				return "string";
			case V8Type::OBJECT:
				return "object";
			case V8Type::ARRAY:
				return "array";
			case V8Type::ARRAY_BUFFER:
				return "array_buffer";
			case V8Type::FUNCTION:
				return "function";
			case V8Type::PROMISE:
				return "promise";
		}
		E_ASSERT(false && "Unknown v8 type");
	}

	[[nodiscard]] APIUsage::APIUsage(APIFunction func):
		func(std::move(func)) {
	}

	[[nodiscard]] constexpr std::string APIUsage::getUsage() const noexcept {
		if (func.name.empty()) {
			return "unknown usage";
		}
		std::ostringstream ret;
		if (!func.path.empty()) {
			ret << func.path << "#";
		}
		ret << func.name << "(";
		const auto args = typesToString(func.expectedArgs);
		ret << util::string::join(args, ", ") << ")";
		if (func.returnType) {
			ret << ": " << stringifyUnionType(*func.returnType);
		}
		return ret.str();
	}

	void APIUsage::validateOrThrow(const CefV8ValueList& arguments) const
		noexcept(false) {
		if (auto err = validateArgs(arguments)) {
			throw std::runtime_error(*err);
		}
	};

	[[nodiscard]] std::optional<std::string>
	APIUsage::validateArgs(const CefV8ValueList& arguments) const noexcept {
		if (arguments.size() < func.expectedArgs.size()
			|| (func.expectedArgs.size() != arguments.size()
				&& !func.allowTrailingArgs)) {
			return std::format("expected {} {} arguments, got {}",
							   func.allowTrailingArgs ? "at least" : "exactly",
							   func.expectedArgs.size(),
							   arguments.size());
		}
		for (auto i = 0; i < arguments.size(); i++) {
			const auto& arg = arguments[i];
			if (getV8Type(arguments[i]) != func.expectedArgs[i]) {
				goto err;
			}
		}
		return {};
err:
		return std::format("Invalid usage. Expected {}. Got {}",
						   getUsage(),
						   makeActualUsageString(arguments));
	}

	[[nodiscard]] std::string APIUsage::makeActualUsageString(
		const CefV8ValueList& arguments) const noexcept {
		std::string ret;
		if (!func.path.empty()) {
			ret += func.path + "#";
		}
		if (func.name.empty()) {
			ret += "<func_name>";
		} else {
			ret += func.name;
		}
		ret += "(";
		ret += util::string::join(
			util::iter::map(arguments,
							[](const CefRefPtr<CefV8Value>& arg) {
								return getTypeName(getV8Type(arg));
							}),
			", ");
		ret += ")";
		if (func.returnType) {
			ret += ": " + stringifyUnionType(*func.returnType);
		}
		return ret;
	}

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<V8Type>& types) noexcept {
		return util::iter::map(types, [](const V8Type& type) -> std::string {
			return getTypeName(type);
			;
		});
	}

	[[nodiscard]] constexpr std::vector<std::string>
	APIUsage::typesToString(const std::vector<uint64_t>& types) noexcept {
		return util::iter::map(types, [](uint64_t type) -> std::string {
			return stringifyUnionType(type);
		});
	}

	[[nodiscard]] constexpr std::string
	APIUsage::stringifyUnionType(uint64_t type) noexcept {
		if (!type) {
			return {};
		}
		std::vector<V8Type> types;
		for (auto i = 1ULL; i < 1ULL << 63 && type; i <<= 1) {
			if (type & i) {
				type &= ~i;
				types.push_back(static_cast<V8Type>(i));
			}
		}
		return util::string::join(typesToString(types), " | ");
	}

	CefRefPtr<CBHandler> CBHandler::Create(Callback h) {
		CefRefPtr<CBHandler> ret = new CBHandler;
		ret->setCallback(std::move(h));
		return ret;
	}

	bool CBHandler::Execute(CB_HANDLER_ARGS) {
		return handler(name, object, arguments, retval, exception);
	}

	void CBHandler::setCallback(Callback h) {
		handler = std::move(h);
	}

	ScopedV8Context::ScopedV8Context(CefRefPtr<CefV8Context> context):
		id(nextId++),
		context(std::move(context)) {
		if (!this->context->IsValid()) {
			E_ASSERT(
				false
				&& "trying to use ScopedV8Context with an invalid context");
		}
		if (CefV8Context::GetEnteredContext()->IsSame(this->context)) {
			shouldExit = false;
		} else {
			context->Enter();
		}
		logger.trace(
			"Entering ScopedV8Context {}, shouldExit: {}", id, shouldExit);
	}

	ScopedV8Context::~ScopedV8Context() {
		if (shouldExit) {
			context->Exit();
		}
		logger.trace(
			"Exiting ScopedV8Context {}, shouldExit: {}", id, shouldExit);
	}

	log::Logger ScopedV8Context::logger {
		{"Extendify", "api", "ScopedV8Context"}};
	int ScopedV8Context::nextId = 1;

	int FilePicker::nextId = 1;
	log::Logger FilePicker::logger {{"Extendify", "api", "FilePicker"}};

	FilePicker::FilePicker(FilePickerData data):
		mode(data.mode) {
	}

	[[nodiscard]] CefRefPtr<FilePicker>
	FilePicker::Create(FilePickerData data) {
		return base::MakeRefCounted<FilePicker>(std::move(data));
	}

	// NOLINTNEXTLINE(performance-unnecessary-value-param)
	[[nodiscard]] CefRefPtr<CefV8Value>
	FilePicker::launch(CefRefPtr<CefV8Context> _context, Callback _callback) {
		this->context = std::move(_context);
		this->callback = std::move(_callback);
		ScopedV8Context ctx(context);
		auto promise = CefV8Value::CreatePromise();
		this->promise = promise;
		runFilePicker();
		return promise;
	}

	void FilePicker::runFilePicker() {
		pickerThread =
			CefThread::CreateThread(std::format("FilePicker-{}", nextId++));
		pickerThread->GetTaskRunner()->PostTask(
			util::TaskCBHandler::Create([this]() {
				auto res = showDialog();
				logger.info("done: {}", res.transform([](std::optional<std::filesystem::path> path) {
					return path.transform(util::into<std::string>);
				}));
			}));
	}

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

		using w_FileDialog_ptrT =
			std::unique_ptr<IFileDialog, decltype([](IFileDialog* ptr) {
								if (ptr) {
									ptr->Release();
								}
							})>;

		class w_FileDialogEvents:
			public std::unique_ptr<IFileDialogEvents,
								   decltype([](IFileDialogEvents* ptr) {
									   if (ptr) {
										   ptr->Release();
									   }
								   })> {
		  public:
			HRESULT code = 0;

			constexpr static w_FileDialogEvents Create() {
				w_FileDialogEvents events;
				IFileDialogEvents* tmp = nullptr;
				events.code =
					CDialogEventHandler::CreateInstance(IID_PPV_ARGS(&tmp));
				events.reset(tmp);
				return events;
			}
		};

		class w_ShellItem:
			public std::unique_ptr<IShellItem, decltype([](IShellItem* ptr) {
									   if (ptr) {
										   ptr->Release();
									   }
								   })> {
		  public:
			HRESULT code {};
		};

		class w_FileDialog: public w_FileDialog_ptrT {

		  public:
			HRESULT code = 0;

			w_FileDialog():
				w_FileDialog_ptrT(nullptr) {
			}

			w_FileDialog(const w_FileDialog& other) = delete;
			w_FileDialog& operator=(const w_FileDialog& other) = delete;

			w_FileDialog(w_FileDialog&& other) noexcept:
				w_FileDialog_ptrT(std::move(other)),
				advCookies(std::move(other.advCookies)) {
			}

			w_FileDialog& operator=(w_FileDialog&& other) noexcept {
				if (this != &other) {
					w_FileDialog_ptrT::operator=(std::move(other));
					advCookies = std::move(other.advCookies);
				}
				return *this;
			}

			~w_FileDialog() {
				for (auto& cookie : advCookies) {
					get()->Unadvise(*cookie);
				}
			}

			constexpr static w_FileDialog Create() {
				w_FileDialog dialog;
				IFileDialog* tmp = nullptr;
				dialog.code = CoCreateInstance(CLSID_FileOpenDialog,
											   nullptr,
											   CLSCTX_INPROC_SERVER,
											   IID_PPV_ARGS(&tmp));
				dialog.reset(tmp);
				return dialog;
			}

			HRESULT Advise(w_FileDialogEvents& pfde, DWORD* pdwCookie) {
				return Advise(pfde.get(), pdwCookie);
			}

			HRESULT Advise(IFileDialogEvents* pfde, DWORD* pdwCookie) {
				E_ASSERT(pfde && "pfde is null");
				E_ASSERT(pdwCookie && "pdwCookie is null");
				E_ASSERT(this->get() && "FileDialog is not initialized");
				advCookies.insert(pdwCookie);
				return this->get()->Advise(pfde, pdwCookie);
			}

			[[nodiscard]] w_ShellItem GetResult() {
				E_ASSERT(this->get() && "FileDialog is not initialized");
				w_ShellItem ret;
				IShellItem* tmp = nullptr;
				ret.code = this->get()->GetResult(&tmp);
				ret.reset(tmp);
				return ret;
			}

		  private:
			std::unordered_set<DWORD*> advCookies;
		};

	} // namespace

	std::expected<std::optional<std::filesystem::path>, std::string>
	showDialog() {
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
		// CoCreate the File Open Dialog object.
		auto pfd = w_FileDialog::Create();

		E_CHECK_ERR(pfd.code);
		// Create an event handling object, and hook it up to the dialog.
		auto pfde = w_FileDialogEvents::Create();
		E_CHECK_ERR(pfde.code);
		DWORD dwCookie;
		// Hook up the event handler.
		HRESULT hr = pfd.Advise(pfde, &dwCookie);
		E_CHECK_ERR(hr);
		// Set the options on the dialog.
		DWORD dwFlags;
		// Before setting, always get the options first in order
		// not to override existing options.
		hr = pfd->GetOptions(&dwFlags);
		E_CHECK_ERR(hr);
		// In this case, get shell items only for file system items.
		hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
		E_CHECK_ERR(hr);
		// Set the file types to display only.
		// Notice that this is a 1-based array.
		constexpr static COMDLG_FILTERSPEC c_rgSaveTypes[] = {
			{L"Style Sheets", L"*.css;*.theme.css"},
			{L"All Files", L"*.*"},
		};
		hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
		E_CHECK_ERR(hr);
		// Set the selected file type index to Style Sheets for this example.
		hr = pfd->SetFileTypeIndex(1); // 1 is the index of "Style Sheets"
		E_CHECK_ERR(hr);
		// Set the default extension to be ".css" file.
		hr = pfd->SetDefaultExtension(L"css");
		E_CHECK_ERR(hr);
		// Show the dialog
		hr = pfd->Show(nullptr);
		E_CHECK_ERR(hr);
		// Obtain the result once the user clicks
		// the 'Open' button.
		// The result is an IShellItem object.
		auto psiResult = pfd.GetResult();
		E_CHECK_ERR(psiResult.code);
		// get the file path
		PWSTR _pszFilePath = nullptr;
		hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &_pszFilePath);
		E_CHECK_ERR(hr);
		std::wstring filePath(_pszFilePath);
		CoTaskMemFree(_pszFilePath);
		return filePath;

#undef E_CHECK_ERR
#endif
	}

	[[nodiscard]] CefRefPtr<FilePicker> FilePicker::pickOne() {
		return FilePicker::Create(FilePickerData {
			.mode = DialogType::OPEN,
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
} // namespace Extendify::api
