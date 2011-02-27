package com.sigmusic.tacchi.tuio;

import android.app.Instrumentation;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;

public class InstHackService extends Service {
	private NotificationManager mNM;

    // Unique Identification Number for the Notification.
    // We use it on Notification start, and to cancel it.
    private int NOTIFICATION = 12312;

    /**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        InstHackService getService() {
            return InstHackService.this;
        }
    }

    @Override
    public void onCreate() {
        mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

        // Display a notification about us starting.  We put an icon in the status bar.
        showNotification();
        Toast.makeText(this, "Local service started", Toast.LENGTH_SHORT).show();
        
        new Thread(new Runnable() {
        	public void run() {
        		insertCrap();
        	}
        }).start();

    }
    
    public void insertCrap() {
        Instrumentation inst = new Instrumentation();
    	while (true) {
            inst.sendKeyDownUpSync( KeyEvent.KEYCODE_C );
            inst.sendKeyDownUpSync( KeyEvent.KEYCODE_R );
            inst.sendKeyDownUpSync( KeyEvent.KEYCODE_A );
            inst.sendKeyDownUpSync( KeyEvent.KEYCODE_P );
            Log.d("Crap", "Crap");
    	}
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i("LocalService", "Received start id " + startId + ": " + intent);
        // We want this service to continue running until it is explicitly
        // stopped, so return sticky.
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        // Cancel the persistent notification.
        mNM.cancel(NOTIFICATION);

        // Tell the user we stopped.
        Toast.makeText(this, "Local service stopped", Toast.LENGTH_SHORT).show();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    // This is the object that receives interactions from clients.  See
    // RemoteService for a more complete example.
    private final IBinder mBinder = new LocalBinder();

    /**
     * Show a notification while this service is running.
     */
    private void showNotification() {
//        // In this sample, we'll use the same text for the ticker and the expanded notification
//        CharSequence text ="Service Started";
//
//        // Set the icon, scrolling text and timestamp
//        Notification notification = new Notification(R.drawable.icon, text,
//                System.currentTimeMillis());
//
////        // The PendingIntent to launch our activity if the user selects this notification
////        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
////                new Intent(this, this.class), 0);
//
//        // Set the info for the views that show in the notification panel.
////        notification.setLatestEventInfo(this, "Something awesome",
////                       text, contentIntent);
//
//        // Send the notification.
//        mNM.notify(NOTIFICATION, notification);
    }
}
