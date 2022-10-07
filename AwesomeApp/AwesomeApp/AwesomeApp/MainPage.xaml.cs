using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Xamarin.Forms;
using System.Net.Mqtt;




namespace AwesomeApp
{
    public partial class MainPage : ContentPage
    {

        private IMqttClient _mqttClient;

        public MainPage()
        {
            InitializeComponent();
        }

        int count = 0;
        void Button_Clicked(object sender, System.EventArgs e)
        {
            count++;
            ((Button)sender).Text = $"You clicked {count} times.";
        }
    }
}
