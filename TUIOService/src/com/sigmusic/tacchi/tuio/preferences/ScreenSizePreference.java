/* The following code was written by Matthew Wiggins 
 * and is released under the APACHE 2.0 license 
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 */
package com.sigmusic.tacchi.tuio.preferences;

import com.sigmusic.tacchi.tuio.TuioServiceTools;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.Typeface;
import android.preference.Preference;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.CompoundButton.OnCheckedChangeListener;

public class ScreenSizePreference extends Preference {
	private static final String androidns = "http://schemas.android.com/apk/res/android";


	public ScreenSizePreference(final Context context) {
		super(context);
	}

	public ScreenSizePreference(final Context context, final AttributeSet attrs) {
		super(context, attrs);
	}

	public ScreenSizePreference(final Context context, final AttributeSet attrs, final int defStyle) {
		super(context, attrs, defStyle);
	}

	@Override
	protected View onCreateView(final ViewGroup parent) {		
		final RelativeLayout layout = new RelativeLayout(getContext());
		return layout;
		
	}

	
	
}
