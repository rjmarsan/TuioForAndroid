package android.view;

import android.os.Binder;
import android.os.IBinder;
import android.view.KeyEvent;
import android.view.MotionEvent;

public interface IWindowManager {
    public static class Stub {
        public static IWindowManager asInterface( IBinder binder ) {
            return null;
        }
    }

    public boolean injectKeyEvent( KeyEvent keyEvent, boolean f );
    public boolean injectPointerEvent( MotionEvent motionEvent, boolean f );
    public boolean injectInputEventNoWait(in InputEvent ev);

}