import QtQuick
import QtQuick.Controls
import QtMultimedia
import DE1App

Page {
    id: screensaverPage
    objectName: "screensaverPage"
    background: Rectangle { color: "black" }

    // Apple TV aerial screensaver videos from Internet Archive (tvOS 10, 1080p)
    // Source: https://archive.org/details/apple-wallpapers-and-screensavers
    readonly property string videoBaseUrl: "https://archive.org/download/apple-wallpapers-and-screensavers/tvOS/tvOS%2010/"
    readonly property var videoFiles: [
        "b1-1.mp4",      // San Francisco
        "b1-2.mp4",      // San Francisco
        "b2-1.mp4",      // New York
        "b2-2.mp4",      // New York
        "b3-1.mp4",      // Hawaii
        "b4-1.mp4",      // China
        "b5-1.mp4",      // London
        "b6-1.mp4",      // Dubai
        "b7-1.mp4",      // Liwa
        "b8-1.mp4",      // Greenland
        "b9-1.mp4",      // Los Angeles
        "b10-1.mp4"      // Hong Kong
    ]

    property int currentVideoIndex: Math.floor(Math.random() * videoFiles.length)

    Component.onCompleted: {
        // Start with a random video
        currentVideoIndex = Math.floor(Math.random() * videoFiles.length)
        mediaPlayer.source = videoBaseUrl + videoFiles[currentVideoIndex]
        mediaPlayer.play()
    }

    MediaPlayer {
        id: mediaPlayer
        audioOutput: AudioOutput { volume: 0 }  // Muted
        videoOutput: videoOutput

        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia) {
                // Play next video
                currentVideoIndex = (currentVideoIndex + 1) % videoFiles.length
                source = videoBaseUrl + videoFiles[currentVideoIndex]
                play()
            } else if (mediaStatus === MediaPlayer.InvalidMedia ||
                       mediaStatus === MediaPlayer.NoMedia) {
                // Try next video if current fails
                currentVideoIndex = (currentVideoIndex + 1) % videoFiles.length
                source = videoBaseUrl + videoFiles[currentVideoIndex]
                play()
            }
        }

        onErrorOccurred: {
            console.log("Video error, trying next:", error)
            currentVideoIndex = (currentVideoIndex + 1) % videoFiles.length
            source = videoBaseUrl + videoFiles[currentVideoIndex]
            play()
        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
    }

    // Fallback: show a subtle animation if video fails
    Rectangle {
        id: fallbackBackground
        anchors.fill: parent
        visible: mediaPlayer.playbackState !== MediaPlayer.PlayingState
        color: "black"

        // Subtle gradient animation as fallback
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Qt.hsla(gradientHue, 0.3, 0.1, 1.0)
                }
                GradientStop {
                    position: 1.0
                    color: Qt.hsla((gradientHue + 0.5) % 1.0, 0.3, 0.05, 1.0)
                }
            }

            property real gradientHue: 0.6

            NumberAnimation on gradientHue {
                from: 0
                to: 1
                duration: 60000
                loops: Animation.Infinite
            }
        }
    }

    // Clock display
    Text {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 50
        text: Qt.formatTime(currentTime, "hh:mm")
        color: "white"
        opacity: 0.8
        font.pixelSize: 80
        font.weight: Font.Light

        property date currentTime: new Date()

        Timer {
            interval: 1000
            running: true
            repeat: true
            onTriggered: parent.currentTime = new Date()
        }
    }

    // Touch hint (fades out)
    Text {
        id: touchHint
        anchors.centerIn: parent
        text: "Touch to wake"
        color: "white"
        opacity: 0.5
        font.pixelSize: 24

        OpacityAnimator {
            target: touchHint
            from: 0.5
            to: 0
            duration: 3000
            running: true
        }
    }

    // Touch anywhere to wake
    MouseArea {
        anchors.fill: parent
        onClicked: wake()
        onPressed: wake()
    }

    // Also wake on key press
    Keys.onPressed: wake()

    function wake() {
        // Stop video
        mediaPlayer.stop()

        // Wake up the DE1
        if (DE1Device.connected) {
            DE1Device.wakeUp()
        }

        // Re-enable auto-scan and try to reconnect to scale
        BLEManager.setAutoScanForScale(true)
        BLEManager.tryDirectConnectToScale()

        // Navigate back to idle
        root.goToIdle()
    }

    // Auto-wake when DE1 wakes up externally (button press on machine)
    Connections {
        target: DE1Device
        function onStateChanged() {
            // If DE1 wakes up externally, wake the app too
            var state = DE1Device.stateString
            if (state !== "Sleep" && state !== "GoingToSleep") {
                mediaPlayer.stop()
                root.goToIdle()
            }
        }
    }
}
