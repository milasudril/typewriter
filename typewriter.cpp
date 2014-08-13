#ifdef __WAND__
target[type[application]name[typewriter]platform[;GNU/Linux]]
#endif

#include <mustudio/client.h>
#include <mustudio/midi_input_exported.h>
#include <mustudio/midi_output_exported.h>
#include <mustudio/midi_event.h>
#include <herbs/event/event.h>

#include <cstdio>
#include <cstring>
#include <deque>
#include <random>

class Typewriter:public MuStudio::Client
	{
	public:
		struct EventQueued
			{
			MuStudio::MIDI::Event event;
			bool valid;
			};

		Typewriter(Herbs::Event& trigg):Client("Typewriter")
			,m_trigg(trigg)
			,midi_in(*this,"Trigger in")
			,midi_out(*this,"MIDI out")
			,event_next{{0,{0}},0},pos(0)
			{
			activate();
			}

		void eventSend(const EventQueued& event)
			{
			events_in.push_back(event);
			}

		int onProcess(size_t n_frames)
			{
			MuStudio::MIDI::Event trigg_in;
			bool event_has=midi_in.eventFirstGet(trigg_in,n_frames);

			midi_out.messageWritePrepare(n_frames);
			size_t now=0;
			while(n_frames)
				{
				if(event_has && trigg_in.time==now)
					{
					//Watch for "note on" message on any channel
					if((trigg_in.data.bytes[0]&0xf0) == 0x90)
						{
						switch(trigg_in.data.bytes[1])
							{
						//TODO: configurable trigger!
							case 64:
								m_trigg.set();
								break;
						//	Spex-haxx, drain que on any other event to prevent
						//	continued typing
							default:
								while(!events_in.empty())
									{events_in.pop_front();}
								break;
							}
						}
					event_has=midi_in.eventNextGet(trigg_in);
					}

				if(pos>=event_next.event.time)
					{
					if(event_next.valid)
						{
						midi_out.messageWrite(event_next.event);
						event_next.valid=0;
						pos=0;
						}
					while(!events_in.empty())
						{
						event_next=events_in.front();
						event_next.valid=1;
						events_in.pop_front();
						if(event_next.event.time==0)
							{
							midi_out.messageWrite(event_next.event);
							event_next.valid=0;
							}
						else
							{break;}
						}
					}
				++pos;
				--n_frames;
				++now;
				}
			return 0;
			}

		~Typewriter()
			{
			deactivate();
			}



	private:
		Herbs::Event& m_trigg;
		MuStudio::MIDI::InputExported midi_in;
		MuStudio::MIDI::OutputExported midi_out;
		std::deque<EventQueued> events_in;
		EventQueued event_next;
		size_t pos;
	};

void errlog(const char* message)
	{fprintf(stderr,"%s\n",message);}


int main(int argc,char* argv[])
	{
	MuStudio::Client::setErrorHandler(errlog);
	Herbs::Event trigger(Herbs::Event::ResetMode::AUTO,Herbs::Event::State::NORMAL);
	Typewriter output(trigger);
	int ch_in;
	double delay=0.150;
	double delay_prev=delay;
	double v_0=0.75;
	double r_prev=0;
	int col_count=0;
	int margin_state=0;
	std::mt19937 twister;
	twister.seed(time(0));
	std::uniform_real_distribution<double> U_t(-0.04,0.04);
	if(setlocale(LC_CTYPE,"sv_SE.ISO-8859-1")==NULL)
		{
		perror("setlocale");
		return -1;
		}
	FILE* src=fopen(argv[1],"r");
	if(src==NULL)
		{
		perror("fopen");
		return -1;
		}
	trigger.wait();
	while((ch_in=getc(src))!=EOF)
		{
		uint8_t note=0;
		if(col_count==80)
			{
			col_count=0;
			margin_state=1;
			output.eventSend({0,{0x99,39,127,0},1});
			}
		switch(ch_in)
			{
			case ' ':
				note=36;
				delay=0.225;
				if(margin_state==0)
					{
					putchar(' ');
					++col_count;
					}
				else
					{
					col_count=0;
					margin_state=0;
					delay=1.0;
					note=40;
					putchar('\n');
					}
				break;
			case '\n':
				col_count=0;
				margin_state=0;
				delay=1.0;
				note=40;
				putchar('\n');
				break;
			case ';':
				col_count=0;
				margin_state=0;
				getc(src);
				getc(src);
				putchar('\n');
				putchar('\n');
				fflush(stdout);
				note=0;
				trigger.wait();
				break;
			default:
				if(isalpha(ch_in) && isupper(ch_in))
					{
					delay=0.4;
					note=38;
					}
				else
					{
					note=37;
					delay=0.15;
					}
				if(margin_state==0)
					{++col_count;}
				putchar(ch_in);
			}

		if(note)
			{
			double r_temp=0.5*(U_t(twister)+r_prev);
			r_prev=r_temp;
			size_t delay_real=size_t(48000*(r_temp+delay_prev));
			output.eventSend({uint32_t(delay_real),{0x99,note,uint8_t(127*v_0),0},1});
			}

		delay_prev=delay;
		}

	trigger.wait();

	return 0;
	}
