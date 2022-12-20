using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Formatter;
using System.Text;

namespace Cafeteria_app;

public partial class MainPage : ContentPage
{
    public static string riaStatus = "Touch me again daddy...";

    private MqttFactory mqttFactory;
    private IMqttClient mqttClient;
    private MqttClientOptions mqttClientOptions;


    public MainPage()
    {
        InitializeComponent();

        mqttFactory = new MqttFactory();
        mqttClient = mqttFactory.CreateMqttClient();

        mqttClientOptions = new MqttClientOptionsBuilder()
                    .WithClientId(Guid.NewGuid().ToString())
                    .WithTcpServer("test.mosquitto.org", 1883)
                    .Build();
    }

    private async void IsRiaOpenClicked(object sender, EventArgs e)
    {
        var sub = Subscribe();
        await sub;
        UpdateRiaStatusLabel(riaStatus);
    }

    private async void SetRiaOpenClicked(object sender, EventArgs e)
    {
        var pub = Publish();
        publisherLabel.Text = "Publishing";
        await pub;
    }

    private async Task Publish()
    {
        /*
         * This sample pushes a simple application message including a topic and a payload.
         *
         * Always use builders where they exist. Builders (in this project) are designed to be
         * backward compatible. Creating an _MqttApplicationMessage_ via its constructor is also
         * supported but the class might change often in future releases where the builder does not
         * or at least provides backward compatibility where possible.
         */
        try
        {

            var asyncAction = mqttClient.ConnectAsync(mqttClientOptions, CancellationToken.None);
            await asyncAction;

            var applicationMessage = new MqttApplicationMessageBuilder()
                    .WithTopic("Cafeteria")
                    .WithPayload("Hello from app!")
                    .Build();

            await mqttClient.PublishAsync(applicationMessage, CancellationToken.None);

            await mqttClient.DisconnectAsync();

            publisherLabel.Text = "Message published";
        }
        catch (Exception e)
        {
            UpdatePublisherLabel(e.ToString());
        }
    }

    private async Task Subscribe()
    {
        try
        {
            mqttClient.ApplicationMessageReceivedAsync += e =>
            {
                Console.WriteLine("Received application message.");
                riaStatus = Encoding.UTF8.GetString(e.ApplicationMessage.Payload);

                return Task.CompletedTask;
            };

            var asyncAction = mqttClient.ConnectAsync(mqttClientOptions, CancellationToken.None);
            await asyncAction;

            var mqttSubscribeOptions = mqttFactory.CreateSubscribeOptionsBuilder()
                    .WithTopicFilter(
                        f =>
                        {
                            f.WithTopic("Cafeteria");
                        })
                    .Build();

            await mqttClient.SubscribeAsync(mqttSubscribeOptions, CancellationToken.None);

            await mqttClient.DisconnectAsync();
        }
        catch (Exception e)
        {
            UpdateRiaStatusLabel(e.ToString());

        }
    }

    private void UpdateRiaStatusLabel(string status)
    {
        string[] data = status.Split('.');

        if (data[1] == "1")
        {
            RiaStatusLabel.Text = "No :_(";
        }
        else if (data[1] == "0")
        {
            RiaStatusLabel.Text = "Yes :D";
        }
        else
        {
            RiaStatusLabel.Text = status;
        }
    }
    private void UpdatePublisherLabel(string status)
    {
        publisherLabel.Text = status;
    }
}


    //private void connect()
    //{
    //    try
    //    {
    //        var mqttFactory = new MqttFactory();

    //        using (var mqttClient = mqttFactory.CreateMqttClient())
    //        {
    //            var mqttClientOptions = new MqttClientOptionsBuilder().WithTcpServer("test.mosquitto.org",1883).WithProtocolVersion(MqttProtocolVersion.V500).Build();

    //            // In MQTTv5 the response contains much more information.
    //            var response = mqttClient.ConnectAsync(mqttClientOptions, CancellationToken.None);

    //            Console.WriteLine("The MQTT client is connected.");


    //        }
    //    }
    //    catch (Exception e)
    //    {
    //        UpdateRiaStatusLabel(e.ToString());

    //    }

    //    try
    //    {
    //        var mqttFactory = new MqttFactory();

    //        using (var mqttClient = mqttFactory.CreateMqttClient())
    //        {
    //            var mqttClientOptions = new MqttClientOptionsBuilder()
    //                                        .WithClientId(Guid.NewGuid().ToString())
    //                                        .WithTcpServer("test.mosquitto.org", 1883)
    //                                        .Build();

    //            // Setup message handling before connecting so that queued messages
    //            // are also handled properly. When there is no event handler attached all
    //            // received messages get lost.
    //            mqttClient.ApplicationMessageReceivedAsync += e =>
    //            {
    //                Console.WriteLine("Received application message.");
    //                riaStatus = Encoding.UTF8.GetString(e.ApplicationMessage.Payload);

    //                return Task.CompletedTask;
    //            };

    //            mqttClient.ConnectAsync(mqttClientOptions, CancellationToken.None);


    //            var mqttSubscribeOptions = mqttFactory.CreateSubscribeOptionsBuilder()
    //                .WithTopicFilter(
    //                    f =>
    //                    {
    //                        f.WithTopic("Cafeteria");
    //                    })
    //                .Build();

    //            mqttClient.SubscribeAsync(mqttSubscribeOptions, CancellationToken.None);

    //        }
    //    }
    //    catch (Exception e)
    //    {
    //        UpdateRiaStatusLabel(e.ToString());

    //    }

    //    while(true)
    //    {
    //        RiaStatusLabel.Invoke();
    //    }
    //}