#nullable enable

using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Izml.Runtime
{
    public sealed class RuntimeError : Exception
    {
        public RuntimeError(string message) : base(message)
        {
        }
    }

    public sealed class Archive : IDisposable
    {
        private IntPtr _handle;

        private Archive(IntPtr handle)
        {
            _handle = handle;
        }

        public static Archive Open(string path)
        {
            IntPtr handle = NativeOpen(path);
            if (handle == IntPtr.Zero)
            {
                throw new RuntimeError(LastError());
            }
            return new Archive(handle);
        }

        public ushort Version => NativeVersion(_handle);

        public ushort MinVersion => NativeMinVersion(_handle);

        public uint RootCount => NativeRootCount(_handle);

        public byte ReadU8()
        {
            Ensure(NativeReadU8(_handle, out byte value));
            return value;
        }

        public sbyte ReadI8()
        {
            Ensure(NativeReadI8(_handle, out sbyte value));
            return value;
        }

        public ushort ReadU16()
        {
            Ensure(NativeReadU16(_handle, out ushort value));
            return value;
        }

        public short ReadI16()
        {
            Ensure(NativeReadI16(_handle, out short value));
            return value;
        }

        public uint ReadU32()
        {
            Ensure(NativeReadU32(_handle, out uint value));
            return value;
        }

        public int ReadI32()
        {
            Ensure(NativeReadI32(_handle, out int value));
            return value;
        }

        public ulong ReadU64()
        {
            Ensure(NativeReadU64(_handle, out ulong value));
            return value;
        }

        public long ReadI64()
        {
            Ensure(NativeReadI64(_handle, out long value));
            return value;
        }

        public float ReadF32()
        {
            Ensure(NativeReadF32(_handle, out float value));
            return value;
        }

        public double ReadF64()
        {
            Ensure(NativeReadF64(_handle, out double value));
            return value;
        }

        public bool ReadBool()
        {
            Ensure(NativeReadBool(_handle, out int value));
            return value != 0;
        }

        public string ReadString()
        {
            Ensure(NativeReadString(_handle, out IntPtr data, out uint length));
            if (length == 0)
            {
                return string.Empty;
            }
            byte[] bytes = new byte[length];
            Marshal.Copy(data, bytes, 0, (int)length);
            return Encoding.UTF8.GetString(bytes);
        }

        public byte[] ReadBytes()
        {
            Ensure(NativeReadBytes(_handle, out IntPtr data, out uint length));
            if (length == 0)
            {
                return Array.Empty<byte>();
            }
            byte[] bytes = new byte[length];
            Marshal.Copy(data, bytes, 0, (int)length);
            return bytes;
        }

        public void Dispose()
        {
            if (_handle != IntPtr.Zero)
            {
                NativeClose(_handle);
                _handle = IntPtr.Zero;
            }
        }

        private static void Ensure(int ok)
        {
            if (ok == 0)
            {
                throw new RuntimeError(LastError());
            }
        }

        private static string LastError()
        {
            IntPtr text = NativeLastError();
            return text == IntPtr.Zero
                ? "unknown IZML runtime error"
                : (Marshal.PtrToStringUTF8(text) ?? "unknown IZML runtime error");
        }

        [DllImport("izml-runtime", EntryPoint = "izmlrt_open", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr NativeOpen([MarshalAs(UnmanagedType.LPUTF8Str)] string path);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_close", CallingConvention = CallingConvention.Cdecl)]
        private static extern void NativeClose(IntPtr archive);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_version", CallingConvention = CallingConvention.Cdecl)]
        private static extern ushort NativeVersion(IntPtr archive);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_min_version", CallingConvention = CallingConvention.Cdecl)]
        private static extern ushort NativeMinVersion(IntPtr archive);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_root_count", CallingConvention = CallingConvention.Cdecl)]
        private static extern uint NativeRootCount(IntPtr archive);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_u8", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadU8(IntPtr archive, out byte value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_i8", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadI8(IntPtr archive, out sbyte value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_u16", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadU16(IntPtr archive, out ushort value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_i16", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadI16(IntPtr archive, out short value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_u32", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadU32(IntPtr archive, out uint value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_i32", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadI32(IntPtr archive, out int value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_u64", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadU64(IntPtr archive, out ulong value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_i64", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadI64(IntPtr archive, out long value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_f32", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadF32(IntPtr archive, out float value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_f64", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadF64(IntPtr archive, out double value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_bool", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadBool(IntPtr archive, out int value);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_string", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadString(IntPtr archive, out IntPtr data, out uint length);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_read_bytes", CallingConvention = CallingConvention.Cdecl)]
        private static extern int NativeReadBytes(IntPtr archive, out IntPtr data, out uint length);

        [DllImport("izml-runtime", EntryPoint = "izmlrt_last_error", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr NativeLastError();
    }
}
