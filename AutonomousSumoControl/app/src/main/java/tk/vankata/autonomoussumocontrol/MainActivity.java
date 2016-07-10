package tk.vankata.autonomoussumocontrol;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class MainActivity extends Activity implements SensorEventListener {

    private SensorManager sensorManager;
    private Sensor accelerometer;

    private double x,y;

    private TextView command;

    private int gestureMotorsWorking=0;

    private int gestureEyesWorking=0;

    private WifiManager wifi;

    private Resources appResources;

    private Context MyContext;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final Button MotorsMovementButton= (Button) findViewById(R.id.motors);
        final Button EyesMovementButton= (Button) findViewById(R.id.eyes);
        final String[] buttonMotorsText={getString(R.string.motorsButtonNotWorking),getString(R.string.motorsButtonWorking)};
        final String[] buttonEyesText={getString(R.string.eyesButtonNowWorking),getString(R.string.eyesButtonWorking)};
        MotorsMovementButton.setOnClickListener(
                new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                gestureEyesWorking=0;
                EyesMovementButton.setText(buttonEyesText[gestureEyesWorking]);
                gestureMotorsWorking ^= 1;
                MotorsMovementButton.setText(buttonMotorsText[gestureMotorsWorking]);
            }
        });
        EyesMovementButton.setOnClickListener(
                new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                gestureMotorsWorking=0;
                MotorsMovementButton.setText(buttonMotorsText[gestureMotorsWorking]);
                gestureEyesWorking^=1;
                EyesMovementButton.setText(buttonEyesText[gestureEyesWorking]);
            }
        });

        initializeViews();

        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        if (sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)!=null) {
            accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_NORMAL);
        } else {
            Log.e("MainActivity",getString(R.string.NoAccelerometerFound));
            Toast.makeText(MainActivity.this, getString(R.string.NoAccelerometerFound), Toast.LENGTH_SHORT).show();
        }

        MyContext=MainActivity.this;
        wifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
        appResources=MyContext.getResources();
    }

    public void initializeViews() {
        command = (TextView) findViewById(R.id.command);
    }

    protected void onResume() {
        super.onResume();
        sensorManager.registerListener(this,accelerometer,SensorManager.SENSOR_DELAY_NORMAL);
    }

    protected void onPause() {
        super.onPause();
        sensorManager.unregisterListener(this);
    }

    private void checkPermissions() {
        int MY_PERMISSIONS_REQUEST_INTERNET=1;
        if (ContextCompat.checkSelfPermission(MainActivity.this,Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {

            if (ActivityCompat.shouldShowRequestPermissionRationale(MainActivity.this,Manifest.permission.INTERNET)) {
            } else {
                ActivityCompat.requestPermissions(MainActivity.this,new String[]{Manifest.permission.INTERNET},MY_PERMISSIONS_REQUEST_INTERNET);
            }
        }
        int MY_PERMISSIONS_REQUEST_ACCESS_WIFI_STATE=1;
        if (ContextCompat.checkSelfPermission(MainActivity.this,Manifest.permission.ACCESS_WIFI_STATE) != PackageManager.PERMISSION_GRANTED) {

            if (ActivityCompat.shouldShowRequestPermissionRationale(MainActivity.this,Manifest.permission.ACCESS_WIFI_STATE)) {
            } else {
                ActivityCompat.requestPermissions(MainActivity.this,new String[]{Manifest.permission.ACCESS_WIFI_STATE},MY_PERMISSIONS_REQUEST_ACCESS_WIFI_STATE);
            }
        }
        int MY_PERMISSIONS_REQUEST_CHANGE_WIFI_STATE=1;
        if (ContextCompat.checkSelfPermission(MainActivity.this,Manifest.permission.CHANGE_WIFI_STATE) != PackageManager.PERMISSION_GRANTED) {

            if (ActivityCompat.shouldShowRequestPermissionRationale(MainActivity.this,Manifest.permission.CHANGE_WIFI_STATE)) {
            } else {
                ActivityCompat.requestPermissions(MainActivity.this,new String[]{Manifest.permission.CHANGE_WIFI_STATE},MY_PERMISSIONS_REQUEST_CHANGE_WIFI_STATE);
            }
        }
        int MY_PERMISSIONS_REQUEST_ACCESS_FINE_LOCATION=1;
        if (ContextCompat.checkSelfPermission(MainActivity.this,Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {

            if (ActivityCompat.shouldShowRequestPermissionRationale(MainActivity.this,Manifest.permission.ACCESS_FINE_LOCATION)) {
            } else {
                ActivityCompat.requestPermissions(MainActivity.this,new String[]{Manifest.permission.ACCESS_FINE_LOCATION},MY_PERMISSIONS_REQUEST_ACCESS_FINE_LOCATION);
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {

    }

    @Override
    public void onSensorChanged(SensorEvent event) {

        if(gestureMotorsWorking==0&&gestureEyesWorking==0)
        {
            command.setText(getString(R.string.AllGestureControlOff));
            return;
        }
        displayCurrentValues();

        x=event.values[0];
        y=event.values[1];
    }

    public boolean couldNotSend,change;

    public void displayCurrentValues() {
        WifiInfo currentState=wifi.getConnectionInfo();
        if(!currentState.getSSID().equals("\""+getString(R.string.WiFiSSID)+"\""))
        {
            command.setText(getString(R.string.WrongNetwork));
            return;
        }
        command.setText(Long.toString(Math.round(x))+" "+Long.toString(Math.round(y)));
        checkPermissions();
        int mode=0;
        if(gestureMotorsWorking==1) {
            mode = 3;
        }
        if(gestureEyesWorking==1) {
            mode = 4;
        }
        if(mode!=0) {
            SendDataToRobot newListCommands = new SendDataToRobot();
            int[] Values={mode,(int) Math.round(x),(int) Math.round(y)};
            newListCommands.execute(Values);
        }
        if(couldNotSend)
        {
            command.setText(getString(R.string.UnableToSend));
            if(change) {
                Toast.makeText(MainActivity.this, getString(R.string.NotAbleToSend), Toast.LENGTH_SHORT).show();
            }
            return;
        }
    }

    public class SendDataToRobot extends AsyncTask<int[], Void, Void> {
        private final String LOG_TAG = SendDataToRobot.class.getSimpleName();
        private boolean failed=false;
        private int[] keys=appResources.getIntArray(R.array.XXTEAKey);

        @Override
        protected Void doInBackground(int[]... params) {
            if (params.length == 0) {
                return null;
            }
            failed=false;
            try {
                DatagramSocket robot = new DatagramSocket();
                InetAddress robotIP = InetAddress.getByName(getString(R.string.ESP8266IP));
                xxtea(params[0],3,keys);
                String transition=Long.toString(params[0][0])+" "+Long.toString(params[0][1])+" "+Long.toString(params[0][2])+"\n";
                byte[] byteMessage = transition.getBytes();
                DatagramPacket toSendPacket = new DatagramPacket(byteMessage, transition.length(), robotIP, appResources.getInteger(R.integer.UDPCommunicationPort));
                robot.send(toSendPacket);
            } catch (IOException e) {
                failed=true;
                Log.e(LOG_TAG,getString(R.string.UDPSendingError), e);
            }
            return null;
        }

        private void xxtea(int[] v, int n,int key[]) {
            int DELTA = appResources.getInteger(R.integer.XXTEADELTA);
            int y,z,sum;
            int p,rounds,e;
            if (n>1) {          /* Coding Part */
                rounds=6+52/n;
                sum=0;
                z=v[n - 1];
                do{
                    sum+=DELTA;
                    e=(sum>>2)&3;
                    for(p=0;p<n-1;p++) {
                        y=v[p+1];
                        z=v[p]+=(((z>>5^y<<2)+(y>>3^z<<4))^((sum^y)+(key[(p&3)^e]^z)));
                    }
                    y=v[0];
                    z=v[n-1]+=(((z>>5^y<<2)+(y>>3^z<<4))^((sum^y)+(key[(p&3)^e]^z)));
                } while((--rounds)!=0);
            } else if (n<-1) {  /* Decoding Part */
                n=-n;
                rounds=6+52/n;
                sum=rounds*DELTA;
                y=v[0];
                do{
                    e=(sum>>2)&3;
                    for(p=n-1;p>0;p--) {
                        z=v[p-1];
                        y=v[p]-=(((z>>5^y<<2)+(y>>3^z<<4))^((sum^y)+(key[(p&3)^e]^z)));
                    }
                    z=v[n-1];
                    y=v[0]-=(((z>>5^y<<2)+(y>>3^z<<4))^((sum^y)+(key[(p&3)^e]^z)));
                    sum-=DELTA;
                } while((--rounds)!=0);
            }
        }

        @Override
        protected void onPostExecute(Void x)
        {
            boolean mem=couldNotSend;
            couldNotSend=false;
            if(failed)
            {
                couldNotSend=true;
            }
            change=(mem!=couldNotSend);
            return;
        }
    }
}
