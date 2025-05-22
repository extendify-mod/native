#pragma once
// stolen chromium code

#include <filesystem>
#include <map>
#include <vector>

namespace Extendify::util {

#ifdef _WIN32
	typedef std::vector<HANDLE> HandlesToInheritVector;
#elif defined(__linux__) || defined(__APPLE__)
	typedef std::vector<std::pair<int, int>> FileHandleMappingVector;
#endif
#ifdef _WIN32
	using NativeEnvironmentString = std::wstring;
#elif defined(__linux__)
	using NativeEnvironmentString = std::string;
#endif
	using EnvironmentMap = std::map<NativeEnvironmentString, NativeEnvironmentString>;

	// Options for launching a subprocess that are passed to LaunchProcess().
	// The default constructor constructs the object with default options.
	struct LaunchOptions {
#if defined(__linux__) && !defined(__APPLE__)
		// Delegate to be run in between fork and exec in the subprocess (see
		// pre_exec_delegate below)
		class PreExecDelegate {
		  public:
			PreExecDelegate() = default;

			PreExecDelegate(const PreExecDelegate&) = delete;
			PreExecDelegate& operator=(const PreExecDelegate&) = delete;

			virtual ~PreExecDelegate() = default;

			// Since this is to be run between fork and exec, and fork may have happened
			// while multiple threads were running, this function needs to be async
			// safe.
			virtual void RunAsyncSafe() = 0;
		};
#endif

		LaunchOptions();
		LaunchOptions(const LaunchOptions&);
		~LaunchOptions();

		// If true, wait for the process to complete.
		bool wait = false;

		// If not empty, change to this directory before executing the new process.
		std::filesystem::path current_directory;

#ifdef _WIN32
		bool start_hidden = false;

		// Process will be started using ShellExecuteEx instead of CreateProcess so
		// that it is elevated. LaunchProcess with this flag will have different
		// behaviour due to ShellExecuteEx. Some common operations like OpenProcess
		// will fail. Currently the only other supported LaunchOptions are
		// |start_hidden| and |wait|.
		bool elevated = false;

		// Sets STARTF_FORCEOFFFEEDBACK so that the feedback cursor is forced off
		// while the process is starting.
		bool feedback_cursor_off = false;

		// Windows can inherit handles when it launches child processes.
		// See https://blogs.msdn.microsoft.com/oldnewthing/20111216-00/?p=8873
		// for a good overview of Windows handle inheritance.
		//
		// Implementation note: it might be nice to implement in terms of
		// std::optional<>, but then the natural default state (vector not present)
		// would be "all inheritable handles" while we want "no inheritance."
		enum class Inherit {
		};
		Inherit inherit_mode = Inherit::kSpecific;
		HandlesToInheritVector handles_to_inherit;

		// If non-null, runs as if the user represented by the token had launched it.
		// Whether the application is visible on the interactive desktop depends on
		// the token belonging to an interactive logon session.
		//
		// To avoid hard to diagnose problems, when specified this loads the
		// environment variables associated with the user and if this operation fails
		// the entire call fails as well.
		UserTokenHandle as_user = nullptr;

		// If true, use an empty string for the desktop name.
		bool empty_desktop_name = false;

		// If non-null, launches the application in that job object. The process will
		// be terminated immediately and LaunchProcess() will fail if assignment to
		// the job object fails.
		HANDLE job_handle = nullptr;

		// Handles for the redirection of stdin, stdout and stderr. The caller should
		// either set all three of them or none (i.e. there is no way to redirect
		// stderr without redirecting stdin).
		//
		// The handles must be inheritable. Pseudo handles are used when stdout and
		// stderr redirect to the console. In that case, GetFileType() will return
		// FILE_TYPE_CHAR and they're automatically inherited by child processes. See
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682075.aspx
		// Otherwise, the caller must ensure that the |inherit_mode| and/or
		// |handles_to_inherit| set so that the handles are inherited.
		HANDLE stdin_handle = nullptr;
		HANDLE stdout_handle = nullptr;
		HANDLE stderr_handle = nullptr;

		// If set to true, ensures that the child process is launched with the
		// CREATE_BREAKAWAY_FROM_JOB flag which allows it to breakout of the parent
		// job if any.
		bool force_breakaway_from_job_ = false;

		// If set to true, permission to bring windows to the foreground is passed to
		// the launched process if the current process has such permission.
		bool grant_foreground_privilege = false;

		// If set to true, sets a process mitigation flag to disable Hardware-enforced
		// Stack Protection for the process.
		// This overrides /cetcompat if set on the executable. See:
		// https://docs.microsoft.com/en-us/cpp/build/reference/cetcompat?view=msvc-160
		// If not supported by Windows, has no effect. This flag weakens security by
		// turning off ROP protection.
		bool disable_cetcompat = false;
#elif defined(__linux__)
		// Remap file descriptors according to the mapping of src_fd->dest_fd to
		// propagate FDs into the child process.
		FileHandleMappingVector fds_to_remap;
#endif

#if defined(_WIN32) || defined(__linux__)
		// Set/unset environment variables. These are applied on top of the parent
		// process environment.  Empty (the default) means to inherit the same
		// environment. See internal::AlterEnvironment().
		EnvironmentMap environment;

		// Clear the environment for the new process before processing changes from
		// |environment|.
		bool clear_environment = false;
#endif

#if __linux__
		// If non-zero, start the process using clone(), using flags as provided.
		// Unlike in clone, clone_flags may not contain a custom termination signal
		// that is sent to the parent when the child dies. The termination signal will
		// always be set to SIGCHLD.
		int clone_flags = 0;

		// By default, child processes will have the PR_SET_NO_NEW_PRIVS bit set. If
		// true, then this bit will not be set in the new child process.
		bool allow_new_privs = false;

		// Sets parent process death signal to SIGKILL.
		bool kill_on_parent_death = false;

		// File descriptors of the parent process with FD_CLOEXEC flag to be removed
		// before calling exec*().
		std::vector<int> fds_to_remove_cloexec;
#endif

#ifdef __APPLE__
		// Mach ports that will be accessible to the child process. These are not
		// directly inherited across process creation, but they are stored by a Mach
		// IPC server that a child process can communicate with to retrieve them.
		//
		// After calling LaunchProcess(), any rights that were transferred with MOVE
		// dispositions will be consumed, even on failure.
		//
		// See base/apple/mach_port_rendezvous.h for details.
		MachPortsForRendezvous mach_ports_for_rendezvous;

		// Apply a process scheduler policy to enable mitigations against CPU side-
		// channel attacks.
		bool enable_cpu_security_mitigations = false;
#endif

#ifdef __APPLE__
		// When a child process is launched, the system tracks the parent process
		// with a concept of "responsibility". The responsible process will be
		// associated with any requests for private data stored on the system via
		// the TCC subsystem. When launching processes that run foreign/third-party
		// code, the responsibility for the child process should be disclaimed so
		// that any TCC requests are not associated with the parent.
		bool disclaim_responsibility = false;

		// A `ProcessRequirement` that will be used to validate the launched process
		// before it can retrieve `mach_ports_for_rendezvous`.
		std::optional<mac::ProcessRequirement> process_requirement;
#endif

#ifdef __linux__
		// If not empty, launch the specified executable instead of
		// cmdline.GetProgram(). This is useful when it is necessary to pass a custom
		// argv[0].
		std::filesystem::path real_path;

		// If true, start the process in a new process group, instead of
		// inheriting the parent's process group.  The pgid of the child process
		// will be the same as its pid.
		bool new_process_group = false;
#endif
	};
} // namespace Extendify::util
