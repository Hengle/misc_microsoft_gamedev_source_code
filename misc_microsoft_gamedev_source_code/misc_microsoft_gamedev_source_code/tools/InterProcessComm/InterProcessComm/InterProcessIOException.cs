using System;
using System.Runtime.Serialization;

namespace InterProcessComm.InterProcessComm {
	#region Comments
	/// <summary>
	/// 
	/// </summary>
	#endregion
	public class InterProcessIOException : Exception {
		#region Comments
		/// <summary>
		/// 
		/// </summary>
		#endregion
		public bool IsServerAvailable = true;
		#region Comments
		/// <summary>
		/// 
		/// </summary>
		#endregion
		public uint ErrorCode = 0;
		#region Comments
		/// <summary>
		/// 
		/// </summary>
		/// <param name="text"></param>
		#endregion
		public InterProcessIOException(String text) : base(text) {
		}
		#region Comments
		/// <summary>
		/// 
		/// </summary>
		/// <param name="info"></param>
		/// <param name="context"></param>
		#endregion
		protected InterProcessIOException(SerializationInfo info, StreamingContext context) : base(info, context) {
		}
	}
}
