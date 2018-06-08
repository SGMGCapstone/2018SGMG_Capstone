package lch.serverFinder;

import android.util.Log;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Date;
import java.util.Locale;

import lch.properties.PropertyManager;

public class UdpReceiver extends Thread
{
	private static final String TAG = "SC1_UDP_RECEIVER";
	private static final int MAGIC = 0x20121632;

	@Override
	public void run()
	{
		byte buffer[] = new byte[24];
		DatagramPacket packet = new DatagramPacket(buffer, 24);
		DatagramSocket datagramSocket = null;
		Date pickedCheckTime = new Date();
		Date curCheckTime;

		// 서순
		// magic
		// cpu core count
		// cpu clock
		// cpu usage
		int magic;
		int cpu_core_count;
		double cpu_clock;
		double cpu_usage;

		double picked_score = 0.0;
		double cur_score;

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
			catch (SocketTimeoutException ex)
			{
				Log.d(TAG, "timeout!!");
				continue;
			}
			catch (IOException ex)
			{
				ex.printStackTrace();
			}

			curCheckTime = new Date();
			if(curCheckTime.getTime() - pickedCheckTime.getTime() > 1000 * 30)
			{
				picked_score = 0.0;
			}

			if (packet.getLength() != 24)
			{
				// something wrong...
				Log.e(TAG, "packet length isn't 24 bytes - length : " + packet.getLength());
			}
			else
			{
				magic = ByteBuffer.wrap(buffer, 0, 4).order(ByteOrder.LITTLE_ENDIAN).getInt();

				if (magic == MAGIC)
				{
					cpu_core_count = ByteBuffer.wrap(buffer,
													 4,
													 4).order(ByteOrder.LITTLE_ENDIAN).getInt();
					cpu_clock = ByteBuffer.wrap(buffer,
												8,
												8).order(ByteOrder.LITTLE_ENDIAN).getDouble();
					cpu_usage = ByteBuffer.wrap(buffer,
												16,
												8).order(ByteOrder.LITTLE_ENDIAN).getDouble();

					cur_score = cpu_clock * cpu_core_count * (1.0 - cpu_usage / 100.0);
					if(cur_score > picked_score)
					{
						picked_score = cur_score;
						pickedCheckTime = curCheckTime;

						PropertyManager.getInstance().VideoEncodingServerIP.set(packet.getAddress().getHostAddress());
					}

					Log.d(TAG,
						  String.format(Locale.getDefault(),
										"cur_pick : %s / recv ip : %s / cpu_core_count : %d / cpu_clock : %f / cpu_usage : %f",
										PropertyManager.getInstance().VideoEncodingServerIP.get(),
										packet.getAddress().getHostAddress(),
										cpu_core_count,
										cpu_clock,
										cpu_usage));
				}
			}
		}
	}
}
