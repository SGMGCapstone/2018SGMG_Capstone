package lch.serverFinder;

import android.util.Log;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

import lch.properties.PropertyManager;

public class UdpReceiver extends Thread
{
	private static final String TAG = "SC1_UDP_RECEIVER";
	private static final int MAGIC = 0x20121632;
	
	@Override
	public void run()
	{
		int recvData;
		byte buffer[] = new byte[4];
		DatagramPacket packet = new DatagramPacket(buffer, 4);
		DatagramSocket datagramSocket = null;

		try
		{
			datagramSocket = new DatagramSocket(17171, InetAddress.getByName("255.255.255.255"));
			datagramSocket.setSoTimeout(1000);
		}
		catch (SocketException e)
		{
			e.printStackTrace();
			return;
		}
		catch (UnknownHostException e)
		{
			e.printStackTrace();
		}

		while (!isInterrupted())
		{
			try
			{
				datagramSocket.receive(packet);
			}
			catch(SocketTimeoutException ex)
			{
				Log.d(TAG, "timeout!!");
				continue;
			}
			catch (IOException ex)
			{
				ex.printStackTrace();
			}

			if(packet.getLength() != 4)
			{
				// something wrong...
				Log.e(TAG, "packet length isn't 4 bytes");
			}
			else
			{
				recvData = ((buffer[3] & 0xff) << 24) | ((buffer[2] & 0xff) << 16)
						| ((buffer[1] & 0xff) << 8) | (buffer[0] & 0xff);

				Log.d(TAG, String.format("recv data : 0x%X", recvData));

				if(recvData == MAGIC)
				{
					PropertyManager.getInstance().VideoEncodingServerIP.set(packet.getAddress().getHostAddress());
				}
			}
		}
	}
}
