/*
 File: Utils.h
 Created on: 28/01/2025
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTILS_H_
#define _UTILS_H_

// Qt
#include <QLabel>
#include <QPoint>
#include <QTime>

class QSettings;
class QShowEvent;

// C++
#include <vector>

struct Configuration
{
  bool captureEnabled = true;                    /** true to capture desktop and false otherwise. */
  QTime captureTime = QTime(0, 0, 30);           /** time between captures. */
  bool captureVideo = true;                      /** true to capture video, capture screenshots otherwise. */
  int captureVideoFPS = 15;                      /** frames per second when capturing video. */
  bool captureAnimateIcon = true;                /** true to animate the tray when when doing a frame/screenshot capture, false otherwise. */
  int captureMonitor = -1;                       /** -1 to capture all displays, otherwise index of the monitor to capture. */
  QString captureOutputDir;                      /** directory to store captures. */
  int captureScale = 0;                          /** index of the scale combo box. */
  QStringList monitors;                          /** detected monitors list. */
  QByteArray appGeometry;                        /** application geometry. */
  QByteArray appState;                           /** application state. */
  bool cameraEnabled = false;                    /** true if camera enabled and false otherwise. */
  QStringList cameraResolutions;                 /** list of detected camera resolutions. */
  int cameraResolution;                          /** index of selected camera resolution in the resolutions list. */
  QPoint cameraOverlayPosition = QPoint{0, 0};   /** coordinates of the camera overlay in the output image. */
  int cameraOverlayCompositionMode = 0;          /** index in the list of composition modes of the combobox for the camera overlay. */
  int cameraOverlayFixedPosition = 0;            /** index in the list of fixed positions for the camera overlay. */
  int cameraMask = 0;                            /** index in the list of masks. */
  bool cameraCenterFace = false;                 /** true to track and center the face int he camera image, false otherwise. */
  bool cameraFaceSmooth = true;                  /** smooth face coordinates processing. */
  bool cameraASCIIart = false;                   /** true to convert tha camera image to ASCII art, false otherwise. */
  bool pomodoroEnabled = true;                   /** true to use pomodoros and false otherwise. */
  QTime pomodoroTime = QTime(0, 25, 0);          /** time of pomodoro. */
  QTime pomodoroShortBreak = QTime(0, 5, 0);     /** time of pomodoro short break. */
  QTime pomodoroLongBreak = QTime(0, 15, 0);     /** time of pomodoro long break; */
  int pomodorosBeforeBreak = 4;                  /** number of pomodoros after a long break. */
  bool pomodoroAnimatedIcon = true;              /** true to animate the tray icon when doing a pomodoro, false otherwise. */
  bool pomodoroUseSounds = true;                 /** true to use sounds when doing a pomodoro, false otherwise. */
  bool pomodoroSoundTicTac = false;              /** true to use continuous tic-tac sounds when doing a pomodoro, false otherwise. */
  int pomodorosNumber = 12;                      /** number of pomodoros in a session. */
  bool pomodoroOverlay = true;                   /** true to overlay the pomodoro statistics in the output image. */
  QString pomodoroTask = "Unknown";              /** name of the last pomodoro task. */
  QPoint pomodoroOverlayPosition = QPoint{0, 0}; /** coordinates of the pomodoro overlay in the output image. */
  int pomodoroOverlayFixedPosition = 0;          /** index in the list of fixed positions for the pomodoro overlay. */
  int pomodoroOverlayCompositionMode = 0;        /** index in the list of composition modes of the combobox for the pomodoro overlay. */
  int cameraASCIIArtRamp = 0;                    /** index the ASCII ramp list for the ramp to use. */
  int cameraASCIIArtCharacterSize = 8;           /** size of the font in the ASCII art image. */
  bool timeOverlay = false;                      /** true to overlay the time on the output image. */
  QPoint timeOverlayPosition = QPoint{0, 0};     /** coordinates of the time overlay in the outout image. */
  int timeOverlayFixedPosition = 0;              /** index in the list of fixed positions for the time overlay. */
  int timeOverlayTextSize = 40;                  /** size of the font to use in the time overlay. */

  /** \brief Helper method to load the configuration.
   *
   */
  void load();

  /** \brief Helper method to save the configuration.
   *
   */
  void save();
};


/** \brief Returns the appropiate QSettings object depending on the presence of the INI file.
 *
 */
std::unique_ptr<QSettings> applicationSettings();

/** \brief Helper method that computes the QRect of the time overlay.
 * \param[in] pixelSize Pixel size of the font. 
 * \param[in] p left-top point of the position to compute rect.
 *
 */
QRect computeTimeOverlayRect(int pixelSize, const QPoint &p);

/** \class ClickableHoverLabel
 * \brief ClickableLabel subclass that changes the mouse cursor when hovered.
 *
 */
class ClickableHoverLabel
    : public QLabel
{
    Q_OBJECT
  public:
    /** \brief ClickableHoverLabel class constructor.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableHoverLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief ClickableHoverLabel class constructor.
     * \param[in] text Label text.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableHoverLabel(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief ClickableHoverLabel class virtual destructor.
     *
     */
    virtual ~ClickableHoverLabel();

  signals:
    void clicked();

  protected:
    void mousePressEvent(QMouseEvent *e) override
    {
      emit clicked();
      QLabel::mousePressEvent(e);
    }

    virtual void enterEvent(QEnterEvent *event) override
    {
      setCursor(Qt::PointingHandCursor);
      QLabel::enterEvent(event);
    }

    virtual void leaveEvent(QEvent *event) override
    {
      setCursor(Qt::ArrowCursor);
      QLabel::leaveEvent(event);
    }
};

/** \brief Circular buffer to soften a value.
 * \param[in] T Some sort of numerical value with + and / operations.
 *
 */
template <class T>
class CircularBuffer
{
  public:
    /** \brief Circular buffer class constructor.
     *
     */
    explicit CircularBuffer()
    {}

    /** \brief Add a value to the buffer. 
     * \param[in] value T value.
     *
     */
    void add(const T &value)
    {
      if(!init) initBuffer(value);
      m_data[m_pointer] = value;
      incrementPointer(m_pointer);
    }

    /** \brief Return the softened value for the current buffer.
     *
     */
    T value() const;

    /** \brief Clears the buffers.
     *
     */
    void clear()
    { init = false; }

  private:
    const int BUFFER_SIZE = 5;

    /** \brief Helper method that increments the given circular buffer pointer. 
     * \param[inout] pointer buffer index.
     *
     */
    void incrementPointer(int &pointer) const
    { auto other = ++pointer % BUFFER_SIZE; pointer = other; }

    /** \brief Helper method that decrements the given circular buffer pointer. 
     * \param[inout] pointer buffer index.
     *
     */
    void decrementPointer(int &pointer) const
    { --pointer; if(pointer < 0) pointer = BUFFER_SIZE-1; }

    /** \brief Initializes the data buffer with the given value.
     * \param[in] initVal Initial buffer value.
     *
     */
    void initBuffer(const T &initVal)
    {
      if(!init) m_data = std::vector<T>(BUFFER_SIZE, initVal);
      init = true;
    }

    std::vector<T> m_data = std::vector<T>(BUFFER_SIZE, T()); /** Circular data buffer. */
    int m_pointer = 0;                       /** Circular current element pointer. */
    bool init = false;                       /** true if initialized and false otherwise. */
};

//---------------------------------------------------------------------
template <class T>
inline T CircularBuffer<T>::value() const
{
  auto pointer = m_pointer;
  int addedValue = 5;
  int accumulation = 0;
  T resultValue;
  for(int i = 0; i < BUFFER_SIZE; ++i)  
  {
    decrementPointer(pointer);
    const auto multiplier = std::max(1, addedValue--);
    accumulation += multiplier;
    resultValue += multiplier * m_data[pointer];
  }
     
  return resultValue / accumulation;
}

#endif // UTILS_H_